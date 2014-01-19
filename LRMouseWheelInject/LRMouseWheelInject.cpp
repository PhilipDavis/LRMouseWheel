//
// Copyright 2010 (c) Philip Davis
//

// LRMouseWheelInject.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "LRMouseWheelInject.h"
#include "Lightroom.h"

#include <psapi.h>
#include <tchar.h>


DWORD GetLightroomProcessId();
void InfiltrateLightroom( DWORD hLightroomProcess );


//
// Entry point of the EXE portion of the plugin.
//
// Workflow goes like this:
//   0. User installs plugin and declares it enabled
//   1. Lightroom runs one of the batch files (depending on what operation is being requested)
//   2. The batch file calls this EXE and passes a command (see stdafx.cpp for commands)
//   3. This EXE injects a DLL into Lightroom's address space (if it hasn't done so already)
//   4. This EXE then calls an entry point on the remote DLL, passing along a coded command
//   5. The remote DLL then takes action based on the command (i.e. enabling or disabling the mouse wheel)
//
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
	//
	// Lightroom is in control of running this part of the plugin.
	// So, when using a DEBUG build, prompt the user(me) to attach a debugger
	// to the process in order to be able to single step through the opening
	// code path. If you don't care about debugging this then just dismiss
	// the dialog.
	//
	if( !IsDebuggerPresent() )
		MessageBox( 0, L"Time to attach to process...", L"LRMouseWheelInject", MB_OK );
#endif


	// Read the command line parameters ( <command> <DllPath> )
	LPWSTR wszPath = NULL;
	LPCWSTR wszCommand = wcstok_s( lpCmdLine, L" ", &wszPath );

	// Strip quotes if necessary
	if( wszPath && wszPath[ 0 ] == L'"' )
	{
		wszPath++;
		int cchPath = wcslen( wszPath );
		if( cchPath > 0 && wszPath[ cchPath - 1 ] == L'"' )
			wszPath[ cchPath - 1 ] = L'\0';
	}

	//
	// If a path wasn't supplied on the command line then default to the current directory
	// TODO: should actually use the same directory as the current process...
	//
	if( wcslen( wszPath ) == 0 )
	{
		WCHAR wszModuleDirectory[ MAX_PATH * 2 + 1 ];
		if( GetModuleFileNameW( NULL, wszModuleDirectory, sizeof( wszModuleDirectory ) / sizeof( WCHAR ) ) > 0 )
		{
			LPWSTR wszLastPathSeparator = wcsrchr( wszModuleDirectory, '\\' );
			if( wszLastPathSeparator )
				*wszLastPathSeparator = L'\0';
			wszPath = wszModuleDirectory;
		}
	}

	//
	// Get the process ID of Lightroom (bail out if it's not running)
	//
	DWORD pidLightroom = GetLightroomProcessId();
	if( !pidLightroom )
		return ERROR_MOD_NOT_FOUND;

	//
	// Initialize our Lightroom wrapper class
	//
	CLightroom lightroom( pidLightroom, wszPath );
	if( !lightroom.Initialize() )
		return ERROR_DLL_INIT_FAILED;

	//
	// Check which command the Lightroom plugin sent to us
	//
	if( !_tcsnicmp( lpCmdLine, wszCommandConnect, 32 ) )
	{
		if( !lightroom.LoadDll() )
			return GetLastError();
	}
	else if( !_tcsnicmp( lpCmdLine, wszCommandDisconnect, 32 ) )
	{
		if( !lightroom.UninstallHooks() && !lightroom.UnloadDll() )
			return GetLastError();
	}
	else if( !_tcsnicmp( lpCmdLine, wszCommandUninstallHooks, 32 ) )
	{
		if( !lightroom.UninstallHooks() )
			return GetLastError();
	}
	else //if( !_tcsnicmp( lpCmdLine, wszCommandInstallHooks, 32 ) )
	{
		// Install hooks by default

		if( !lightroom.InstallHooks() )
			return GetLastError();
	}

	return NOERROR;
}

//
// Helper method to tell us if a given process ID corresponds to Lightroom
//
bool IsLightroomProcess( DWORD processId )
{
	bool isLightroomProcess = false;

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, processId );
	if( hProcess )
	{
		WCHAR wszFilename[ MAX_PATH + 1 ];

		if( GetProcessImageFileNameW( hProcess, wszFilename, MAX_PATH ) > 0 )
			isLightroomProcess = ( wcsstr( wszFilename, wszLightroomExe ) != NULL );

		CloseHandle( hProcess );
	}

	return isLightroomProcess;
}

//
// Helper method to find Lightroom's process ID
//
DWORD GetLightroomProcessId()
{
	// For sheer laziness, assume there are no more than 1024 running processes
	// Obviously would spend more time on production code :)
	DWORD processIds[ 1024 ];
	DWORD cb = 0;
	if( EnumProcesses( processIds, sizeof( processIds ), &cb ) )
	{
		DWORD nProcesses = cb / sizeof( processIds[ 0 ] );
		for( DWORD i = 0; i < nProcesses; i++ )
			if( IsLightroomProcess( processIds[ i ] ) )
				return processIds[ i ];
	}

	return NULL;
}
