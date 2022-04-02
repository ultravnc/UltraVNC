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
