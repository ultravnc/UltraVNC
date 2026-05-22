// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#ifdef SC_20
#include "loadDllFromMemory.h"
#include "windows.h"
#include "./winvnc/resource.h"

LoadDllFromMemory::LoadDllFromMemory()
{
	dDengineBuf = NULL;
	handleDDengine = NULL;
	hDsmpluginDll = NULL;
	szTempDsmPlugin[0] = '\0';
};

LoadDllFromMemory::~LoadDllFromMemory()
{
	if (dDengineBuf != NULL)
		free(dDengineBuf);
	if (handleDDengine != NULL)
		MemoryFreeLibrary(handleDDengine);
	if (hDsmpluginDll != NULL) {
		FreeLibrary(hDsmpluginDll);
		hDsmpluginDll = NULL;
	}
	if (szTempDsmPlugin[0] != '\0') {
		DeleteFileA(szTempDsmPlugin);
		szTempDsmPlugin[0] = '\0';
	}
}

void LoadDllFromMemory::loadDDengine(StartW8Fn& StartW8, StartW8V2Fn& StartW8V2, StopW8Fn& StopW8, LockW8Fn& LockW8, UnlockW8Fn& UnlockW8, ShowCursorW8Fn& ShowCursorW8, HideCursorW8Fn& HideCursorW8)
{
	HRSRC hResource = FindResource(hInstResDLL, MAKEINTRESOURCE(IDR_DLL1), "DLL");
	HGLOBAL hFileResource = LoadResource(hInstResDLL, hResource);

	// Open and map this to a disk file
	LPVOID lpFile = LockResource(hFileResource);
	DWORD dwSize = SizeofResource(hInstResDLL, hResource);
	dDengineBuf = new BYTE[dwSize];
	CopyMemory(dDengineBuf, lpFile, dwSize);
	handleDDengine = MemoryLoadLibrary(dDengineBuf, dwSize);

	if (handleDDengine) {
		StartW8 = (StartW8Fn)MemoryGetProcAddress(handleDDengine, "StartW8");
		StartW8V2 = (StartW8V2Fn)MemoryGetProcAddress(handleDDengine, "StartW8V2");
		StopW8 = (StopW8Fn)MemoryGetProcAddress(handleDDengine, "StopW8");

		LockW8 = (LockW8Fn)MemoryGetProcAddress(handleDDengine, "LockW8");
		UnlockW8 = (UnlockW8Fn)MemoryGetProcAddress(handleDDengine, "UnlockW8");

		ShowCursorW8 = (ShowCursorW8Fn)MemoryGetProcAddress(handleDDengine, "ShowCursorW8");
		HideCursorW8 = (HideCursorW8Fn)MemoryGetProcAddress(handleDDengine, "HideCursorW8");
	}
}

void LoadDllFromMemory::LoadPlugin(DESCRIPTION& m_PDescription, SHUTDOWN& m_PShutdown, STARTUP& m_PStartup, SETPARAMS& m_PSetParams, GETPARAMS& m_PGetParams,
	TRANSFORMBUFFER& m_PTransformBuffer, RESTOREBUFFER& m_PRestoreBuffer, FREEBUFFER& m_PFreeBuffer, RESET& m_PReset,
	CREATEPLUGININTERFACE& m_PCreatePluginInterface, CREATEINTEGRATEDPLUGININTERFACE& m_PCreateIntegratedPluginInterface, CONFIG& m_PConfig)
{
	HRSRC hResource = FindResource(hInstResDLL, MAKEINTRESOURCE(IDR_ENCRYPTIONDLL1), "ENCRYPTIONDLL");
	HGLOBAL hFileResource = LoadResource(hInstResDLL, hResource);

	// Open and map this to a disk file
	LPVOID lpFile = LockResource(hFileResource);
	DWORD dwSize = SizeofResource(hInstResDLL, hResource);

	// Extract embedded plugin to a temp file and load via LoadLibraryA.
	// MemoryLoadLibrary cannot handle static TLS (__declspec(thread)) used by OpenSSL 3.x.
	char szTempPath[MAX_PATH];
	char szTempBase[MAX_PATH];
	GetTempPathA(MAX_PATH, szTempPath);
	GetTempFileNameA(szTempPath, "vnc", 0, szTempBase);
	DeleteFileA(szTempBase);
	strcpy_s(szTempDsmPlugin, MAX_PATH, szTempBase);
	strcat_s(szTempDsmPlugin, MAX_PATH, ".dsm");

	HANDLE hFile = CreateFileA(szTempDsmPlugin, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwWritten = 0;
		WriteFile(hFile, lpFile, dwSize, &dwWritten, NULL);
		CloseHandle(hFile);
	}

	hDsmpluginDll = LoadLibraryA(szTempDsmPlugin);
	if (!hDsmpluginDll) return;

	m_PDescription = (DESCRIPTION)GetProcAddress(hDsmpluginDll, "Description");
	m_PStartup = (STARTUP)GetProcAddress(hDsmpluginDll, "Startup");
	m_PShutdown = (SHUTDOWN)GetProcAddress(hDsmpluginDll, "Shutdown");
	m_PSetParams = (SETPARAMS)GetProcAddress(hDsmpluginDll, "SetParams");
	m_PGetParams = (GETPARAMS)GetProcAddress(hDsmpluginDll, "GetParams");
	m_PTransformBuffer = (TRANSFORMBUFFER)GetProcAddress(hDsmpluginDll, "TransformBuffer");
	m_PRestoreBuffer = (RESTOREBUFFER)GetProcAddress(hDsmpluginDll, "RestoreBuffer");
	m_PFreeBuffer = (FREEBUFFER)GetProcAddress(hDsmpluginDll, "FreeBuffer");
	m_PReset = (RESET)GetProcAddress(hDsmpluginDll, "Reset");
	m_PCreatePluginInterface = (CREATEPLUGININTERFACE)GetProcAddress(hDsmpluginDll, "CreatePluginInterface");
	m_PCreateIntegratedPluginInterface = (CREATEINTEGRATEDPLUGININTERFACE)GetProcAddress(hDsmpluginDll, "CreateIntegratedPluginInterface");
	m_PConfig = (CONFIG)GetProcAddress(hDsmpluginDll, "Config");
}
#endif