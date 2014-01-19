//
// Copyright 2010 (c) Philip Davis
//

// stdafx.cpp : source file that includes just the standard includes
// LRMouseWheelInject.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"


#ifndef _DEBUG
const LPWSTR wszLightroomExe = L"lightroom.exe";
#else
// Sometimes I test injection with calc.exe for conveniece (it loads faster :)
//const LPWSTR wszLightroomExe = L"calc.exe";
const LPWSTR wszLightroomExe = L"lightroom.exe";
#endif

const LPWSTR wszLrMouseWheelDll = L"LRMouseWheel.dll";

const LPSTR szEntryPoint = "_EntryPoint@4";


//
// Commands that will be sent from the batch files
//
const LPWSTR wszCommandConnect = L"connect";
const LPWSTR wszCommandDisconnect = L"disconnect";
const LPWSTR wszCommandInstallHooks = L"enable";
const LPWSTR wszCommandUninstallHooks = L"disable";