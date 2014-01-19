//
// Copyright 2010 (c) Philip Davis
//
// CLightroomGuest is the API for manipulating Lightroom from within Lightroom's address
// space (hence 'guest'). It is the core of the DLL that is injected into Lightroom.
// The bulk of this class deals with scanning for windows that belong to Scroll Views
// and subclasses those windows so it can process their mouse wheel messages.
// (It would literally take Adobe just a few lines of code to make this work :)
//

#include "StdAfx.h"
#include "LightroomGuest.h"

#include <stack>

const LPWSTR wszClipView = L"ClipView";
const LPWSTR wszScrollView = L"Scroll View";


/* TODO

- Figure out why Lightroom isn't starting the plugin at startup
	- I think this is fixed now.  Still need to do lots of testing with enabling/disabling/startup/shutdown
	- looks like the catalog has trouble starting up (it seems blocked until I open Plugin Manager)
- Debug issue with scrolling on Import dialog (and test other dialogs too)
  - It seemed to get stuck in a back-and-forth scrolling infinite loop
- Also, each new dialog gets assigned new window handles... so need to make sure old values are removed from maps!
	- So need to know when a dialog disappears
		- could subclass the dialog window and watch for close

*/


CLightroomGuest *CLightroomGuest::pInstance = 0;

CLightroomGuest::CLightroomGuest() :
	isInstalled( false )
{
	InitializeCriticalSection( &criticalSection );
	InitializeCriticalSection( &csMaps );
}

CLightroomGuest::~CLightroomGuest()
{
	sentinelThread.Exit();
	DeleteCriticalSection( &criticalSection );
	DeleteCriticalSection( &csMaps );
}

CLightroomGuest *CLightroomGuest::GetInstance()
{
	if( !pInstance )
		pInstance = new CLightroomGuest();

	return pInstance;
}

void CLightroomGuest::DeleteInstance()
{
	delete pInstance;
	pInstance = 0;
}

HWND CLightroomGuest::getCurrentPanelHost()
{
	HWND hWndLightroom = FindWindowW( L"AgWinMainFrame", NULL );
	if( !hWndLightroom )
		return NULL;


	/* Temporarily commented out because the scroll hooking isn't working properly yet
	// Note: The Import dialog is a child window with the caption "Lightroom"
	//       We should look for a popup dialog before we look for a panel host
	//       in the main app window because the user will be dealing with the dialog.
	// TODO: verify that the caption is always "Lightroom"... maybe it's more accurate to say that
	//       the window caption will always match that of the main window.
	HWND hWndChildPopup = FindWindowExW( NULL, NULL, L"Afx:00400000:0", L"Lightroom" );
	if( hWndChildPopup )
	{
		//log( L"Found child dialog window 0x%X\r\n", hWndChildPopup );
		return hWndChildPopup; // pretend that it's a panel host
	}
	*/

	// We didn't find a child popup window... so continue looking in the main app window

	HWND hWndUnnamed = GetWindow( hWndLightroom, GW_CHILD );
	if( !hWndUnnamed )
		return NULL;

	HWND hWndTopBottomPanelHost = GetWindow( hWndUnnamed, GW_CHILD );
	if( !hWndTopBottomPanelHost )
		return NULL;

	HWND hWndMainSwitcherView = FindWindowExW( hWndTopBottomPanelHost, NULL, L"AfxWnd100u", L"Main Switcher View" );
	if( !hWndMainSwitcherView )
		return NULL;

	return GetWindow( hWndMainSwitcherView, GW_CHILD );
}

//
// I found out that Lightroom doesn't create all the windows at the beginning.
// (e.g. you start in Library panel and then jump to Develop panel... then all of Develop is created)
// So added this helper method to re-scan the current panel host for scrollable windows
//
void CLightroomGuest::FindNewWindows()
{
	EnterCriticalSection( &criticalSection );

	// Find out which panel host is currently visible
	HWND hWndCurrentPanelHost = getCurrentPanelHost();
	if( hWndCurrentPanelHost )
	{
		EnterCriticalSection( &csMaps );

		// If we haven't seen this Panel Host before then search it for child windows to hook
		if( panelHosts.find( hWndCurrentPanelHost ) == panelHosts.end() )
		{
			log( L"Noticed new Panel Host 0x%X\r\n", hWndCurrentPanelHost );
			panelHosts.insert( hWndCurrentPanelHost );
			LeaveCriticalSection( &csMaps );

			hookWindows( hWndCurrentPanelHost );
		}
		else
			LeaveCriticalSection( &csMaps );
	}

	LeaveCriticalSection( &criticalSection );
}

//
// Enable mouse wheel functionality
//
DWORD CLightroomGuest::InstallWindowHooks()
{
	log( L"InstallWindowHooks()\r\n" );

	EnterCriticalSection( &criticalSection );

	if( !isInstalled )
	{
		isInstalled = true;

		// Ensure that the maps are clear (in case the user toggles the enabled flag a few times)
		EnterCriticalSection( &csMaps );
		scrollViewToScrollBarMap.clear();
		childToScrollViewMap.clear();
		childToWndProcMap.clear();
		scrollViews.clear();
		LeaveCriticalSection( &csMaps );

		// Discover all the Scroll Views, Clip Views and Scroll Bars
		FindNewWindows();

		sentinelThread.Resume();

		log( L"InstallWindowHooks() - done\r\n" );
	}
	else log( L"InstallWindowHooks() - was already done\r\n" );


	sentinelThread.Enable();

	LeaveCriticalSection( &criticalSection );
	return 0;
}

//
// Enumerate all the windows and hook into the scroll view children.
//
void CLightroomGuest::hookWindows( HWND hWndParent )
{
	if( !hWndParent )
		return;

	EnumChildWindows( hWndParent, CLightroomGuest::_enumWindows, ( LPARAM )this );

	// Some of the Scroll Bars are actually on the window above the Scroll View
	// So look for Clip Views that don't have an associated Scroll Bar and try to find a Scroll Bar.
	findSpecialCaseScrollBars();

	// Hook into the child windows of each Scroll View
	EnterCriticalSection( &csMaps );
	stdext::hash_map< HWND, HWND >::const_iterator iter;
	for( iter = childToScrollViewMap.begin(); iter != childToScrollViewMap.end(); iter++ )
	{
		HWND hWndChild = iter->first;
		HWND hWndScrollView = iter->second;
		if( !hWndScrollView )
			continue;

		// If we haven't already hooked this window then
		if( childToWndProcMap.find( hWndChild ) == childToWndProcMap.end() )
		{
			// Subclass the child window and store the old window proc
			LONG fnOldWndProc = GetWindowLong( hWndChild, GWL_WNDPROC );
			childToWndProcMap[ hWndChild ] = fnOldWndProc;
			SetWindowLong( hWndChild, GWL_WNDPROC, ( LONG )CLightroomGuest::_wndProc );
		}
	}
	LeaveCriticalSection( &csMaps );
}

void CLightroomGuest::findSpecialCaseScrollBars()
{
	EnterCriticalSection( &csMaps );

	std::hash_set< HWND >::const_iterator iter;
	for( iter = scrollViews.begin(); iter != scrollViews.end(); iter++ )
	{
		HWND hWndScrollView = *iter;

		// If we don't have a Scroll Bar mapping for this Scroll View then
		if( scrollViewToScrollBarMap.find( hWndScrollView ) == scrollViewToScrollBarMap.end() )
		{
			// Attempt to get a scroll bar belonging to the parent of the Scroll View
			HWND hWndParent = GetParent( hWndScrollView );
			HWND hWndScrollBar = GetWindow( hWndParent, GW_CHILD );
			while( hWndScrollBar )
			{
				if( isScrollBarWindow( hWndScrollBar ) )
				{
					log( L"Found special Scroll Bar window 0x%X (for Scroll View 0x%X)\r\n", hWndScrollBar, hWndScrollView );
					scrollViewToScrollBarMap.insert( std::pair< HWND, HWND >( hWndScrollView, hWndScrollBar ) );
					childToScrollViewMap[ hWndScrollView ] = hWndScrollView; // Point the scroll view to itself
					break;
				}
				hWndScrollBar = GetWindow( hWndScrollBar, GW_HWNDNEXT );
			}
		}
	}

	LeaveCriticalSection( &csMaps );
}

//
// Callback method that will locate for us all the windows that can be scrolled.
// Keep track of a bunch of things like which windows are scroll views, which
// scroll bars are associated with which scroll views, and which child windows
// belong to which scroll views. The idea here is that, when we detect a mouse
// wheel event on a child window, we can quickly determine where that even needs
// to be sent.
//
BOOL CLightroomGuest::_enumWindows( HWND hWnd, LPARAM lParam )
{
	// The Windows way to pass a class instance into a static method
	CLightroomGuest *pThis = ( CLightroomGuest* )lParam;

	/* I'm not sure why I had this code in before.  By only dealing with visible windows, we fail to hook panels that have been hidden by the user.
	if( !IsWindowVisible( hWnd ) || !IsWindowEnabled( hWnd ) )
		return TRUE;
	*/

	// Get the window title -- Note: this will send a WM_GETTEXT message... so need to be careful with already-hooked windows
	const int cchWindowTitle = 32; // (Don't care about longer strings)
	WCHAR wszWindowTitle[ cchWindowTitle ];
	bool hasWindowText = ( GetWindowTextW( hWnd, wszWindowTitle, cchWindowTitle ) != 0 );

	// If this window is a Scroll View then store it
	if( hasWindowText && pThis->isScrollViewWindow( hWnd, wszWindowTitle, cchWindowTitle ) )
	{
		// We've found a Scroll View window! Add it to the set
		log( L"Found Scroll View window 0x%X\r\n", hWnd );
		EnterCriticalSection( &pThis->csMaps );
		pThis->scrollViews.insert( hWnd );
		LeaveCriticalSection( &pThis->csMaps );
	}

	// If the window is a Scroll Bar, store it and bail out
	else if( pThis->isScrollBarWindow( hWnd ) )
	{
		HWND hWndParent = GetParent( hWnd );
		log( L"Found Scroll Bar window 0x%X (for Scroll View 0x%X)\r\n", hWnd, hWndParent );
		EnterCriticalSection( &pThis->csMaps );
		pThis->scrollViewToScrollBarMap.insert( std::pair< HWND, HWND >( hWndParent, hWnd ) );
		LeaveCriticalSection( &pThis->csMaps );
	}

	else if( hasWindowText && pThis->isClipViewWindow( hWnd, wszWindowTitle, cchWindowTitle ) )
	{
		// Find out what the parent Scroll View is (if any)
		log( L"Found ClipView window 0x%X\r\n", hWnd );
		// KILL: pThis->findParentScrollView( hWnd );

		HWND hWndParent = GetParent( hWnd );

		EnterCriticalSection( &pThis->csMaps );
		pThis->childToScrollViewMap[ hWnd ] = hWndParent;
		LeaveCriticalSection( &pThis->csMaps );
	}

	// System class windows (listbox, edit, combobox) seem to not propagate the 
	// WM_MOUSEWHEEL to their parent window.  So subclass these windows directly.
	else if( pThis->isSystemWindow( hWnd ) )
	{
		// Walk up the parent chain until we find a Scroll View
		log( L"Found system window 0x%X\r\n", hWnd );
		HWND hWndChild = hWnd;
		HWND hWndParent;
		while( ( hWndParent = GetParent( hWnd ) ) != NULL )
		{
			WCHAR wszWindowTitle[ cchWindowTitle ];
			if( GetWindowTextW( hWndParent, wszWindowTitle, cchWindowTitle ) )
			{
				if( pThis->isScrollViewWindow( hWndParent, wszWindowTitle, cchWindowTitle ) )
				{
					// Found the scroll view, so add it to our map and bail out
					log( L"Assigning system window 0x%X to Scroll View 0x%X\r\n", hWndChild, hWndParent );
					EnterCriticalSection( &pThis->csMaps );
					pThis->childToScrollViewMap[ hWndChild ] = hWndParent;
					LeaveCriticalSection( &pThis->csMaps );
					break;
				}
			}
			hWnd = hWndParent;
		}
	}

	//else log( L"Ignoring unknown window 0x%X\r\n", hWnd );


	return TRUE; // Continue enumerating windows
}

bool CLightroomGuest::isClipViewWindow( HWND hWnd, LPCWSTR wszWindowTitle, const int cchWindowTitle )
{
	return( _wcsnicmp( wszWindowTitle, wszClipView, cchWindowTitle ) == 0 );
}

bool CLightroomGuest::isScrollViewWindow( HWND hWnd, LPCWSTR wszWindowTitle, const int cchWindowTitle )
{
	return( _wcsnicmp( wszWindowTitle, wszScrollView, cchWindowTitle ) == 0 );
}

bool CLightroomGuest::isScrollBarWindow( HWND hWnd )
{
	const int cchClassName = 10; // (Don't care about longer strings)
	WCHAR wszClassName[ cchClassName ];

	if( !RealGetWindowClass( hWnd, wszClassName, cchClassName ) )
		return false;

	if( _wcsnicmp( wszClassName, L"ScrollBar", cchClassName ) )
		return false;

	// There are two scroll bars, but we only care about the one that is visible.  Unfortunately,
	// it's not as simple as checking for visibility or being enabled.  Check the window size for
	// the one that is not 0-width (or 0-height)

	RECT rc;
	if( !GetWindowRect( hWnd, &rc ) )
		return false;

	// Bail out if the window has no height or width
	if( !( rc.bottom - rc.top ) || !( rc.right - rc.left ) )
		return false;

	return true;
}

bool CLightroomGuest::isSystemWindow( HWND hWnd )
{
	const int cchClassName = 18; // (Don't care about longer strings)
	WCHAR wszClassName[ cchClassName ];

	if( !RealGetWindowClass( hWnd, wszClassName, cchClassName ) )
		return false;

	if( _wcsnicmp( wszClassName, L"SysListView32", 14 ) == 0 )
		return true;

	if( _wcsnicmp( wszClassName, L"Edit", 5 ) == 0 )
		return true;
	
	if( _wcsnicmp( wszClassName, L"ComboBox", 9 ) == 0 )
		return true;

	if( _wcsnicmp( wszClassName, L"msctls_trackbar32", 18 ) == 0 )
		return true;

	return false;
}

//
// Disable mouse wheel functionality
//
DWORD CLightroomGuest::UninstallWindowHooks()
{
	log( L"UninstallWindowHooks()\r\n" );

	EnterCriticalSection( &criticalSection );

	sentinelThread.Disable();

	if( isInstalled )
	{
		isInstalled = false;

		EnterCriticalSection( &csMaps );
		std::hash_map< HWND, LONG >::const_iterator iter;
		for( iter = childToWndProcMap.begin(); iter != childToWndProcMap.end(); iter++ )
			SetWindowLong( iter->first, GWL_WNDPROC, iter->second );
		LeaveCriticalSection( &csMaps );

		// Note: we can't erase the map yet because there may still be threads relying on getting valid values out of it.
		//       So we'll clear the collection in the InstallWindowHooks function instead.

		log( L"UninstallWindowHooks() - done\r\n" );
	}
	else log( L"UninstallWindowHooks() - not necessary\r\n" );

	LeaveCriticalSection( &criticalSection );
	return 0;
}

//
// This is our window proc that receives all events for hooked windows.
// Deal with mouse wheel messages and defer all other messages to the next window in the chain.
//
LRESULT CALLBACK CLightroomGuest::_wndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	CLightroomGuest *pLightroomGuest = GetInstance();

	switch( message )
	{
	case WM_MOUSEWHEEL:
		log( L"WM_MOUSEWHEEL - 0x%X, %d, %d, %d, %d\r\n", hWnd, HIWORD( wParam ), LOWORD( wParam ), HIWORD( lParam ), LOWORD( lParam ) );
		pLightroomGuest->ForwardMouseWheel( hWnd, message, wParam, lParam );
		return 0;
	}

	return pLightroomGuest->CallPreviousWindowProc( hWnd, message, wParam, lParam );
}

//
// Helper function to forward an event to the hooked window.
// We have already stored the window handle and address in a map
// so look up the address by window handle and call it.
//
LRESULT CALLBACK CLightroomGuest::CallPreviousWindowProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	EnterCriticalSection( &csMaps );

	std::hash_map< HWND, LONG >::const_iterator iter = childToWndProcMap.find( hWnd );
	if( iter == childToWndProcMap.end() )
	{
		LeaveCriticalSection( &csMaps );
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	WNDPROC fnWndProc = ( WNDPROC )iter->second;
	
	LeaveCriticalSection( &csMaps );

	return fnWndProc( hWnd, message, wParam, lParam );
}

//
// Send a captured mouse wheel event to the scroll bar associated with the content
// (which is typically on some parent window)
//
void CLightroomGuest::ForwardMouseWheel( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	EnterCriticalSection( &csMaps );

	// Find the ancestor Scroll View window
	std::hash_map< HWND, HWND >::const_iterator iter = childToScrollViewMap.find( hWnd );
	if( iter != childToScrollViewMap.end() )
	{
		// Bail out if there is no Scroll View parent (should not happen since we wouldn't have subclassed such a window...)
		HWND hWndScrollView = iter->second;
		if( hWndScrollView )
		{
			// Find the Scroll Bar window associated with the ancestor Scroll View
			iter = scrollViewToScrollBarMap.find( hWndScrollView );
			if( iter != scrollViewToScrollBarMap.end() )
			{
				// Send the message to the Scroll Bar
				HWND hWndScrollBar = iter->second;
				
				LeaveCriticalSection( &csMaps );

				PostMessageW( hWndScrollBar, message, wParam, lParam );
				return;
			}
		}
	}

	LeaveCriticalSection( &csMaps );
}
