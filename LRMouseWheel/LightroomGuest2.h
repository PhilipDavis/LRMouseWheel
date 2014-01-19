//
// Copyright 2010 (c) Philip Davis
//

#pragma once

class CLightroomGuest2
{
	CRITICAL_SECTION criticalSection;

	bool isInstalled; // TRUE if our hooks are in
	HHOOK hHook;

	static CLightroomGuest2 *pInstance;

	CLightroomGuest2();
	~CLightroomGuest2();

	static LRESULT CALLBACK _mouseProc( int nCode, WPARAM wParam, LPARAM lParam );

	static HWND getScrollBarForWindow( HWND hWnd );
	static bool isScrollBarWindow( HWND hWnd );


public:
	static CLightroomGuest2 *GetInstance();
	static void DeleteInstance();

	DWORD InstallWindowHooks();
	DWORD UninstallWindowHooks();
};

