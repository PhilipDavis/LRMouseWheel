//
// Copyright 2010 (c) Philip Davis
//

#pragma once

class CLightroom
{
	HANDLE hProcess;
	DWORD processId;
	DWORD pRemoteDllBase;
	DWORD pRemoteEntryPoint;
	LPCWSTR wszPath;

	bool getRemoteDllBase();
	bool calculateRemoteEntryPoint();
	
	DWORD executeCommand( DWORD command );

public:
	CLightroom( DWORD processId, LPCWSTR wszDllPath );
	~CLightroom();

	bool Initialize();

	bool LoadDll();
	bool UnloadDll();

	// Remote commands
	DWORD InstallHooks();
	DWORD UninstallHooks();
};

