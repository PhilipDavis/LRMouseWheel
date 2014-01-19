//
// Copyright 2010 (c) Philip Davis
//
// CLightroomGuest2 was a failed partial attempt to capture mouse wheel messages in a cleaner,
// simpler manner. However, I got occupied with other things and didn't have time to get this
// to work (or to conclude it can't work)
//

#include "StdAfx.h"
#include "LightroomGuest2.h"


const LPWSTR wszScrollView = L"Scroll View";


/*
Second idea: Perform either a global hook or a local hook on the GUI thread.
Catch mouse wheel messages.  If the mouse pointer is over a scroll bar then don't do anything.
Otherwise, look for sibling windows that are scroll bars.  If none are found then go up to
the parent and start looking for siblings.  Repeat this process until we find an enabled and
visible scroll bar (with non-zero rect size) or until we have no parent window.

*/

/*
Didn't go very well... the call back wasn't called (possibly because the thread exited?)
but now that I think about it... I set it up for a global hook which might require a 
message loop

CLightroomGuest2 *CLightroomGuest2::pInstance = 0;

CLightroomGuest2::CLightroomGuest2() :
	isInstalled( false )
{
	InitializeCriticalSection( &criticalSection );
}

CLightroomGuest2::~CLightroomGuest2()
{
	DeleteCriticalSection( &criticalSection );
}

CLightroomGuest2 *CLightroomGuest2::GetInstance()
{
	if( !pInstance )
		pInstance = new CLightroomGuest2();

	return pInstance;
}

void CLightroomGuest2::DeleteInstance()
{
	delete pInstance;
	pInstance = 0;
}

DWORD CLightroomGuest2::InstallWindowHooks()
{
	DWORD mainThreadId = 0; // Just trap for all threads // TODO: determine which thread is the GUI thread
	
	hHook = SetWindowsHookExW( WH_MOUSE, _mouseProc, _shModule, mainThreadId );
	if( !hHook )
	{
		log( L"Failed to set windows hook: %d\r\n", GetLastError() );
	}

	return( !hHook );
}

DWORD CLightroomGuest2::UninstallWindowHooks()
{
	return UnhookWindowsHookEx( hHook );
}

bool CLightroomGuest2::isScrollBarWindow( HWND hWnd )
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

HWND CLightroomGuest2::getScrollBarForWindow( HWND hWnd )
{
	// Attempt to get a parent scroll view
	HWND hWndParent = GetParent( hWnd );
	while( hWndParent )
	{
		// Get the window title -- Note: this will send a WM_GETTEXT message... so might want to cache lookups
		const int cchWindowTitle = 32; // (Don't care about longer strings)
		WCHAR wszWindowTitle[ cchWindowTitle ];
		if( GetWindowTextW( hWnd, wszWindowTitle, cchWindowTitle ) )
		{
			// Is the window title "Scroll View"?
			if( _wcsnicmp( wszWindowTitle, wszScrollView, cchWindowTitle ) )
			{
				// Found a scroll view!  Now find the scroll bar
				HWND hWndScrollBar = GetWindow( hWndParent, GW_CHILD );
				while( hWndScrollBar )
				{
					if( isScrollBarWindow( hWndScrollBar ) )
					{

						return hWndScrollBar;
					}
					hWndScrollBar = GetWindow( hWndScrollBar, GW_HWNDNEXT );
				}
			}
		}
	}

	return NULL;
}

LRESULT CLightroomGuest2::_mouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	if( nCode == HC_ACTION )
	{
		LPMOUSEHOOKSTRUCT mhs = ( LPMOUSEHOOKSTRUCT )lParam;
		HWND hWndScrollBar = getScrollBarForWindow( mhs->hwnd );
		if( hWndScrollBar && hWndScrollBar != mhs->hwnd ) // if we found a scroll bar and the window itself isn't the scroll bar
		{

			return 1;
		}
	}

	return CallNextHookEx( pInstance->hHook, nCode, wParam, lParam );
}
*/