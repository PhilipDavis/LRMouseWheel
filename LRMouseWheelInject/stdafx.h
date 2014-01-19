//
// Copyright 2010 (c) Philip Davis
//

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>



extern const LPWSTR wszLightroomExe;

extern const LPWSTR wszLrMouseWheelDll;

extern const LPSTR szEntryPoint;


extern const LPWSTR wszCommandConnect;
extern const LPWSTR wszCommandDisconnect;
extern const LPWSTR wszCommandInstallHooks;
extern const LPWSTR wszCommandUninstallHooks;