//
// Copyright 2010 (c) Philip Davis
//

#include "stdafx.h"
#include "LightroomGuest.h"
//#include "LightroomGuest2.h"

//
// This is the entry point in the remote DLL. It is running inside Lightroom's address space.
// The incoming parameter is a command that was passed by the EXE in the plugin's address space.
//
extern "C" __declspec( dllexport ) DWORD WINAPI EntryPoint( LPVOID pParameter )
{
	//
	// CLightroomGuest is the convenience wrapper class to represent Lightoom (within Lightroom's address space).
	// Call methods on CLightroomGuest to affect Lightroom.
	//
	// CLightroomGuest2 was a different method to acheive the same end goal. It is simpler in theory, but
	// I didn't get it working (and didn't spend any more time on it... even though I had some thoughts on
	// how it might be fixed)
	//

	CLightroomGuest *pLightroomGuest = CLightroomGuest::GetInstance();
	//CLightroomGuest2 *pLightroomGuest = CLightroomGuest2::GetInstance();

	switch( ( DWORD )pParameter )
	{
	case 1: // Install window hooks
		return pLightroomGuest->InstallWindowHooks();

	case 2: // Uninstall window hooks
		if( pLightroomGuest->UninstallWindowHooks() == 0 )
			CLightroomGuest::DeleteInstance();
	}

    return 0;
}
