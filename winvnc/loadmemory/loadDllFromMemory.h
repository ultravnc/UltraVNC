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
#pragma once

#include <WinSock2.h>
#include <Windows.h>
#include "MemoryModule.h"
#include "../DSMPlugin/DSMPlugin.h"

typedef bool(*StartW8Fn)(bool);
typedef bool(*StartW8V2Fn)(bool, bool, UINT);
typedef bool (*StopW8Fn)();

typedef void (*LockW8Fn)();
typedef void (*UnlockW8Fn)();

typedef void (*ShowCursorW8Fn)();
typedef void (*HideCursorW8Fn)();
extern HINSTANCE	hInstResDLL;

class LoadDllFromMemory
{
private:
	BYTE* dDengineBuf;
	HMEMORYMODULE handleDDengine;
	BYTE* dsmpluginBuf;
	HMEMORYMODULE handleDsmpluginBuf;
public:
	LoadDllFromMemory();
	~LoadDllFromMemory();

	void loadDDengine(StartW8Fn& StartW8, StartW8V2Fn& StartW8V2, StopW8Fn& StopW8, LockW8Fn& LockW8, UnlockW8Fn& UnlockW8, ShowCursorW8Fn& ShowCursorW8, HideCursorW8Fn& HideCursorW8);
	void LoadPlugin(DESCRIPTION& m_PDescription, SHUTDOWN& m_PShutdown, STARTUP& m_PStartup, SETPARAMS& m_PSetParams, GETPARAMS& m_PGetParams,
		TRANSFORMBUFFER& m_PTransformBuffer, RESTOREBUFFER& m_PRestoreBuffer, FREEBUFFER& m_PFreeBuffer, RESET& m_PReset,
		CREATEPLUGININTERFACE& m_PCreatePluginInterface, CREATEINTEGRATEDPLUGININTERFACE& m_PCreateIntegratedPluginInterface, CONFIG& m_PConfig);
};
#endif
