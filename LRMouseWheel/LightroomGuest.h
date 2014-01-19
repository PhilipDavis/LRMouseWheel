//
// Copyright 2010 (c) Philip Davis
//

#pragma once

#include <hash_map>
#include <hash_set>

#include "SentinelThread.h"


class CLightroomGuest
{
	// Given any window handle, return the scroll view ancestor window (or NULL if no such ancestor exists)
	std::hash_map< HWND, HWND > childToScrollViewMap;

	// Given a Scroll View window handle, return the associated Scroll Bar window handle
	std::hash_map< HWND, HWND > scrollViewToScrollBarMap;

	// Store the window handles of the Scroll Views that we've found
	std::hash_set< HWND > scrollViews;

	// Store all the old window procs
	std::hash_map< HWND, LONG > childToWndProcMap;

	// Each module in Lightroom has its user interface on a panel host.
	// We need to keep track of all the panel hosts we've seen so far because Lightroom only creates
	// a panel when the user accesses the module.  By keeping track of the panel hosts we know about,
	// it will be easy to determine when new ones are added.
	std::hash_set< HWND > panelHosts;

	bool isInstalled; // TRUE if our hooks are in

	CSentinelThread sentinelThread;

	CRITICAL_SECTION criticalSection;
	CRITICAL_SECTION csMaps;

	static CLightroomGuest *pInstance;

	void findParentScrollView( HWND hWnd );
	bool isScrollViewWindow( HWND hWnd, LPCWSTR wszWindowTitle, const int cchWindowTitle );
	bool isScrollBarWindow( HWND hWnd );
	bool isClipViewWindow( HWND hWnd, LPCWSTR wszWindowTitle, const int cchWindowTitle );
	bool isSystemWindow( HWND hWnd );

	void hookWindows( HWND hWndParent );

	HWND getCurrentPanelHost();

	void findSpecialCaseScrollBars();

	CLightroomGuest();
	~CLightroomGuest();

public:
	static CLightroomGuest *GetInstance();
	static void DeleteInstance();

	DWORD InstallWindowHooks();
	DWORD UninstallWindowHooks();

	void FindNewWindows();

private:
	void ForwardMouseWheel( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	LRESULT CALLBACK CallPreviousWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	static LRESULT CALLBACK _wndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	static BOOL CALLBACK _enumWindows( HWND hWnd, LPARAM lParam );
};

