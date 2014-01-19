//
// Copyright (c) 2010, Philip Davis
//
// The Sentinel Thread watches for new windows. It was added once I realized that
// Lightroom lazy loads its modules. E.g. you open up Lightroom into the Library
// module and LRMouseWheel hooks all the existing windows... but then you switch
// to the Develop module and scrolling wouldn't work in any of the Develop panels.
//
// So now, we scan every 2 seconds for new windows. There is minimal overhead.
// Maybe there's a better way though... e.g. hooking WM_CREATE and looking for new modules?
//

#include "StdAfx.h"
#include "SentinelThread.h"
#include "LightroomGuest.h"

// Run the sentinel thread every two seconds
int timeout = 2000;


CSentinelThread::CSentinelThread()
{
	hExitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	hEnabledEvent = CreateEvent( NULL, TRUE, TRUE, NULL );
	hThread = CreateThread( NULL, 0, CSentinelThread::_run, this, CREATE_SUSPENDED, &threadId );
}

CSentinelThread::~CSentinelThread()
{
	Exit();
	CloseHandle( hEnabledEvent );
	CloseHandle( hExitEvent );
	CloseHandle( hThread );
}

void CSentinelThread::Resume()
{
	ResumeThread( hThread );
}

void CSentinelThread::Exit()
{
	SetEvent( hExitEvent );
	DWORD dwWait = WaitForSingleObject( hThread, 5000 ); // Wait for up to five seconds
}

DWORD CSentinelThread::_run( LPVOID pParam )
{
	CSentinelThread *pThis = ( CSentinelThread* )pParam;

	// Wait for the sentinel thread to be enabled or for the exit command
	DWORD dwWait;
	HANDLE hEvents[ 2 ] = { pThis->hExitEvent, pThis->hEnabledEvent };
	while( ( dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, INFINITE ) ) != WAIT_OBJECT_0 )
	{
		// Wait for up to <timeout> milliseconds to receive the exit command
		while( ( dwWait = WaitForSingleObject( pThis->hExitEvent, timeout ) ) == WAIT_TIMEOUT )
			CLightroomGuest::GetInstance()->FindNewWindows();
	}

	return 0;
}

void CSentinelThread::Disable()
{
	ResetEvent( hEnabledEvent );
}

void CSentinelThread::Enable()
{
	SetEvent( hEnabledEvent );
}