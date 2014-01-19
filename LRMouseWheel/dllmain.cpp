//
// Copyright 2010 (c) Philip Davis
//

// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

HMODULE _shModule = 0;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		_shModule = hModule;
		// fall through

	case DLL_THREAD_ATTACH:
		// TODO: could inspect the current thread to decide if it's the GUI thread... (for LightroomGuest2)

	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

