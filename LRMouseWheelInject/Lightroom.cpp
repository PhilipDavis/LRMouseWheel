//
// Copyright (c) 2010, Philip Davis
//
// CLightroom is a wrapper object to abstract away operations I want to perform on Lightroom.
// This class runs in the plugin's address space, but can indirectly manipulate anything in
// the remote (aka Lightroom) address space via the injected DLL. Think of this class as the
// bridge across a worm hole between two universes. :)
//

#include "StdAfx.h"
#include "Lightroom.h"
#include <psapi.h>

//
// Remote commands (I would make this an enum, but VS 2010 keeps crashing when I use an enum)
//
// These are for internal use. Use CLightroom::InstallHooks and CLightroom::UninstallHooks.
//
const DWORD commandInstallHooks = 1;   // Enable scroll wheel
const DWORD commandUninstallHooks = 2; // Disable scroll wheel


CLightroom::CLightroom( DWORD processId, LPCWSTR wszDllPath ) :
	hProcess( 0 ),
	processId( processId ),
	pRemoteDllBase( 0 ),
	pRemoteEntryPoint( 0 ),
	wszPath( wszDllPath )
{}

//
// Finished with the handle to the Lightroom process. Close it now.
//
CLightroom::~CLightroom()
{
	if( hProcess )
		CloseHandle( hProcess );
}

//
// Open a handle to the Lightroom process
// We need to be running at the same or higher priviledge level as Lightroom
//
bool CLightroom::Initialize()
{
	hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, processId );
	return( hProcess != NULL );
}

//
// Find out where we need to send commands to in the remote address space.
// Do this by finding out where the entry point is relative to the beginning
// of the module (using our own address space) and then applying this delta
// to the DLL's base address in the remote space.
//
bool CLightroom::calculateRemoteEntryPoint()
{
	HMODULE hModDll = LoadLibraryW( wszLrMouseWheelDll );
	if( !hModDll )
		return false;

	DWORD pLocalEntryPoint = ( DWORD )GetProcAddress( hModDll, szEntryPoint );

	pRemoteEntryPoint = ( pRemoteDllBase + pLocalEntryPoint - ( DWORD )hModDll );

	FreeLibrary( hModDll );

	return true;
}

//
// Install our DLL in Lightroom's address space
//
bool CLightroom::LoadDll()
{
	bool isSuccess = false;

	// Bail out right away if the DLL is already loaded
	if( getRemoteDllBase() && pRemoteDllBase != 0 )
		return calculateRemoteEntryPoint();

	// Build an absolute path to LrMouseWheel.dll
	size_t cchPath = wcslen( wszPath );
	WCHAR wszFullPath[ MAX_PATH * 2 ];
	wcsncpy_s( wszFullPath, wszPath, cchPath + 1 );
	if( cchPath > 0 )
		wcsncat_s( wszFullPath, L"\\", 1 );
	wcsncat_s( wszFullPath, wszLrMouseWheelDll, _TRUNCATE );

	// Copy the filename into the remote process
	DWORD cbDllFilename = ( wcslen( wszFullPath ) + 1 ) * sizeof( WCHAR );
	LPVOID pRemoteDllFilename = VirtualAllocEx( hProcess, NULL, cbDllFilename, MEM_COMMIT, PAGE_READWRITE );
	if( WriteProcessMemory( hProcess, pRemoteDllFilename, wszFullPath, cbDllFilename, NULL ) )
	{
		// Cause the DLL to be loaded into the remote process
		PTHREAD_START_ROUTINE pEntryPoint = ( PTHREAD_START_ROUTINE )GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "LoadLibraryW" );
		HANDLE hRemoteThread = CreateRemoteThread( hProcess, NULL, 0, pEntryPoint, pRemoteDllFilename, 0, NULL );
		if( hRemoteThread )
		{
			WaitForSingleObject( hRemoteThread, INFINITE );

			// Find out where the DLL was loaded
			isSuccess = ( GetExitCodeThread( hRemoteThread, &pRemoteDllBase ) != 0 ) && pRemoteDllBase != 0;

			// Clean up
			CloseHandle( hRemoteThread );
		}
	}

	VirtualFreeEx( hProcess, pRemoteDllFilename, 0, MEM_RELEASE );

	return isSuccess && calculateRemoteEntryPoint();
}

//
// Convenience method to tell the remote DLL to turn on
//
DWORD CLightroom::InstallHooks()
{
	return executeCommand( commandInstallHooks );
}

//
// Convenience method to tell the remote DLL to turn off (but stay resident)
//
DWORD CLightroom::UninstallHooks()
{
	return executeCommand( commandUninstallHooks );
}

//
// Send a command to the DLL injected in Lightroom's address space
// (and load the DLL first if it hasn't been loaded yet)
//
DWORD CLightroom::executeCommand( DWORD command )
{
	if( !LoadDll() )
		return E_UNEXPECTED;

	DWORD exitCode = 0;

	// Call our entry point and pass in the command
	DWORD threadId = 0;
	HANDLE hRemoteThread = CreateRemoteThread( hProcess, NULL, 0, ( LPTHREAD_START_ROUTINE )pRemoteEntryPoint, ( LPVOID )command, 0, &threadId );
	if( hRemoteThread )
	{
		WaitForSingleObject( hRemoteThread, INFINITE ); // Probably doesn't need to be an infinite wait...
		GetExitCodeThread( hRemoteThread, &exitCode );

		CloseHandle( hRemoteThread );
	}

	return exitCode;
}

//
// Remove our injected DLL from Lightroom's address space
//
bool CLightroom::UnloadDll()
{
	bool isSuccess = false;

	// Bail out if there's nothing to unload
	if( !getRemoteDllBase() )
		return true;

	// Launch a remote thread to free the library
	PTHREAD_START_ROUTINE pEntryPoint = ( PTHREAD_START_ROUTINE )GetProcAddress( GetModuleHandleW( L"kernel32.dll" ), "FreeLibrary" );
	HANDLE hRemoteThread = CreateRemoteThread( hProcess, NULL, 0, pEntryPoint, ( PVOID )pRemoteDllBase, 0, NULL );
	if( hRemoteThread )
	{
		WaitForSingleObject( hRemoteThread, 10000 ); // Only wait a max of 10 seconds for this to complete
		CloseHandle( hRemoteThread );
		isSuccess = true;
	}

	return isSuccess;
}

//
// Find out where our injected DLL is residing in Lightroom's address space,
// store the value in pRemoteDllBase and return a boolean indicating whether
// we were successful or not.
//
bool CLightroom::getRemoteDllBase()
{
	HMODULE hModule[ 1024 ];
	DWORD cb = 0;
	if( EnumProcessModules( hProcess, hModule, sizeof( hModule ), &cb ) )
	{
		DWORD nModules = cb / sizeof( hModule[ 0 ] );
		for( DWORD i = 0; i < nModules && !pRemoteDllBase; i++ )
		{
			WCHAR wszBaseName[ MAX_PATH ];
			if( GetModuleBaseNameW( hProcess, hModule[ i ], wszBaseName, MAX_PATH ) )
			{
				if( _wcsnicmp( wszBaseName, wszLrMouseWheelDll, wcslen( wszLrMouseWheelDll ) ) == 0 )
					pRemoteDllBase = ( DWORD )hModule[ i ];
			}
		}
	}
	return( pRemoteDllBase != 0 );
}
