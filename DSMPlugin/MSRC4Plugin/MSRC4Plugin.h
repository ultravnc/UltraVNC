// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//


//
// IF YOU USE THIS GPL SOURCE CODE TO MAKE YOUR DSM PLUGIN, PLEASE ADD YOUR
// COPYRIGHT TO THE TOP OF THIS FILE AND SHORTLY DESCRIBE/EXPLAIN THE 
// MODIFICATIONS YOU'VE MADE. THANKS.
//
// IF YOU DON'T USE THIS CODE AS A BASE FOR YOUR PLUGIN, THE HEADER ABOVE AND
// ULTRAVNC COPYRIGHT SHOULDN'T BE FOUND IN YOUR PLUGIN SOURCE CODE.
//
////////////////////////////////////////////////////////////////////////////


#ifndef _MSRC4PLUGIN_H
#define _MSRC4PLUGIN_H

#ifdef PLUGIN_EXPORTS
#define PLUGIN_API __declspec(dllexport)
#else
#define PLUGIN_API __declspec(dllimport)
#endif

#pragma once
#ifdef _WITH_LOG  
	#include "logging.h"
#else 
	#define BUFFER_SIZE 256
#endif  

#include <stdlib.h>
#include <memory.h>
#include "resource.h"
#include <crtdbg.h>
#include <windows.h>

#include "EnvReg.h"

#ifdef _WITH_REGISTRY
				// Registry key locations
#define MSRC4_KEY_NAME_VIEWER _T("Software\\ORL\\VNCViewer\\DSMPlugins\\MSRC4")
#define MSRC4_KEY_NAME_SERVER _T("Software\\ORL\\WinVNC3\\DSMPlugins\\MSRC4")

#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

/////////////////////////////////////////////////////////////////////////

// Internal Plugin vars, depending on what it does
char  szExternalKey[255];   // To store the password/key transmitted via SetParams() by UltraVNC apps
char  szLoaderType[52];     // To store the type of application that has loaded the plugin 

BYTE* pLocalTransBuffer = NULL; // Local Transformation buffer (freed on VNC demand)
BYTE* pLocalRestBuffer = NULL;  // Local Restoration buffer (freed on VNC demand)
int   nLocalTransBufferSize = 0;
int   nLocalRestBufferSize = 0;

// *** 2 of everything here. 1 for input, 1 for output.***
HCRYPTPROV    hProvider = 0;                // crypto provider
HCRYPTPROV    hProvider2 = 0;               // crypto provider
HCRYPTKEY     hKey = 0;                     // generated key 
HCRYPTKEY     hKey2 = 0;                    // generated key 
HCRYPTKEY     hExchangeKey = 0;             // user's exchange key
HCRYPTKEY     hExchangeKey2 = 0;            // user's exchange key

//////////////////////////////////////////////////////////////////////////

const char ErrorDialog[] = "The plugin was unable to locate the key file. Please ensure that the registry entry is properly set and that the key file exists.";

#define KEYFILENAME_SIZE 256
#define SALT_SIZE 11
#define IV_SIZE 16

#ifdef _WITH_REGISTRY  
	const char * sDefault = "\\ultravnc\\";
#endif  

const char * sDefaultKeyName = "rc4.key";
const char * sLogItDef = "0";	// Default to off

DWORD maxKeyLen = 0;

// Plugin struct.
typedef struct
{
	char szDescription[256];
	char szDescription2[256];
	char szStatus[3000];
	int nMode;

	char szHKLMServer[KEYFILENAME_SIZE];
	char szHKCUServer[KEYFILENAME_SIZE];

	char szHKCUViewer[KEYFILENAME_SIZE];
	char szHKCUGenKey[KEYFILENAME_SIZE];
	int intKeyLen;

} PLUGINSTRUCT;


//
// Internal local functions
//
BOOL MyStrToken(LPSTR szToken, LPSTR lpString, int nTokenNum, char cSep);
int GiveTransDataLen(int nDataLen);
int GiveRestDataLen(int nDataLen);
BYTE* CheckLocalTransBufferSize(int nBufferSize);
BYTE* CheckLocalRestBufferSize(int nBufferSize);

// Config procs if necessary (the plugin is allowed to have no config dialog at all)
BOOL CALLBACK ConfigDlgProc(HWND hwnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam );
int DoDialog(void);

BOOL CALLBACK ErrorDlgProc(HWND hwnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam );
int DoError(void);

//
// A DSM Plugin MUST export (extern "C" - __cdecl) all the following functions
// (same names, same signatures)

extern "C"
{
PLUGIN_API char* Description(void);
PLUGIN_API int Startup(void);
PLUGIN_API int Shutdown(void);
PLUGIN_API int SetParams(HWND hVNC, char* szParams);
PLUGIN_API char* GetParams(void);
PLUGIN_API BYTE* TransformBuffer(BYTE* pDataBuffer, int nDataLen, int* pnTransformedDataLen);
PLUGIN_API BYTE* RestoreBuffer(BYTE* pTransBuffer, int nTransDataLen, int* pnRestoredDataLen);
PLUGIN_API void FreeBuffer(BYTE* pBuffer);
PLUGIN_API int Reset(void);
}


#endif