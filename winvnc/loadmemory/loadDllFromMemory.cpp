/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#ifdef SC_20
#include "loadDllFromMemory.h"
#include "windows.h"
#include "./winvnc/resource.h"

LoadDllFromMemory::LoadDllFromMemory()
{
	dDengineBuf = NULL;
	handleDDengine = NULL;
	dsmpluginBuf = NULL;
	handleDsmpluginBuf = NULL;
};

LoadDllFromMemory::~LoadDllFromMemory()
{
	if (dDengineBuf != NULL)
		free(dDengineBuf);
	if (handleDDengine != NULL)
		MemoryFreeLibrary(handleDDengine);
	if (dsmpluginBuf != NULL)
		free(dsmpluginBuf);
	if (handleDsmpluginBuf != NULL)
		MemoryFreeLibrary(handleDsmpluginBuf);
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
	dsmpluginBuf = new BYTE[dwSize];
	CopyMemory(dsmpluginBuf, lpFile, dwSize);
	handleDsmpluginBuf = MemoryLoadLibrary(dsmpluginBuf, dwSize);

	m_PDescription = (DESCRIPTION)MemoryGetProcAddress(handleDsmpluginBuf, "Description");
	m_PStartup = (STARTUP)MemoryGetProcAddress(handleDsmpluginBuf, "Startup");
	m_PShutdown = (SHUTDOWN)MemoryGetProcAddress(handleDsmpluginBuf, "Shutdown");
	m_PSetParams = (SETPARAMS)MemoryGetProcAddress(handleDsmpluginBuf, "SetParams");
	m_PGetParams = (GETPARAMS)MemoryGetProcAddress(handleDsmpluginBuf, "GetParams");
	m_PTransformBuffer = (TRANSFORMBUFFER)MemoryGetProcAddress(handleDsmpluginBuf, "TransformBuffer");
	m_PRestoreBuffer = (RESTOREBUFFER)MemoryGetProcAddress(handleDsmpluginBuf, "RestoreBuffer");
	m_PFreeBuffer = (FREEBUFFER)MemoryGetProcAddress(handleDsmpluginBuf, "FreeBuffer");
	m_PReset = (RESET)MemoryGetProcAddress(handleDsmpluginBuf, "Reset");
	m_PCreatePluginInterface = (CREATEPLUGININTERFACE)MemoryGetProcAddress(handleDsmpluginBuf, "CreatePluginInterface");
	m_PCreateIntegratedPluginInterface = (CREATEINTEGRATEDPLUGININTERFACE)MemoryGetProcAddress(handleDsmpluginBuf, "CreateIntegratedPluginInterface");
	m_PConfig = (CONFIG)MemoryGetProcAddress(handleDsmpluginBuf, "Config");
}
#endif