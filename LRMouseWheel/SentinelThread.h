//
// Copyright 2010 (c) Philip Davis
//

#pragma once

class CSentinelThread
{
	static DWORD WINAPI _run( LPVOID pParam );

	DWORD threadId;
	HANDLE hThread;
	HANDLE hExitEvent;
	HANDLE hEnabledEvent;

public:
	CSentinelThread();
	~CSentinelThread();

	void Exit();
	void Resume();
	void Enable();
	void Disable();
};

