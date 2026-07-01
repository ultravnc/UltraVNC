// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// Suppress deprecated API warnings for legacy compatibility
#pragma warning(disable: 4996)

#include "stdhdrs.h"
#include "vncviewer.h"
#include "Exception.h"
#include "display.h"
#include <shlobj.h>
/*#ifdef _INTERNALLIB
#pragma comment(lib, "zlibstat.lib")
#pragma comment(lib, "zip32.lib")
#pragma comment(lib, "unz32lib.lib")
#pragma comment(lib, "libjpeg-turbo-win-static.lib")
#pragma comment(lib, "libzstd_static.lib")
#endif*/

#include "omnithread/omnithread.h"
#include "VNCviewerApp32.h"
#include "shellscalingapi.h"
#include "UltraVNCHelperFunctions.h"

// All logging is done via the log object
Log vnclog;
wchar_t sz_A1[64];
wchar_t sz_A2[64];
wchar_t sz_A3[64];
wchar_t sz_A4[64];
wchar_t sz_A5[64];
wchar_t sz_B1[64];
wchar_t sz_B2[64];
wchar_t sz_B3[64];
wchar_t sz_C1[64];
wchar_t sz_C2[64];
wchar_t sz_C3[64];
wchar_t sz_D1[64];
wchar_t sz_D2[64];
wchar_t sz_D3[64];
wchar_t sz_D4[64];
wchar_t sz_D5[64];
wchar_t sz_D6[64];
wchar_t sz_D7[64];
wchar_t sz_D8[64];
wchar_t sz_D9[64];
wchar_t sz_D10[64];
wchar_t sz_D11[64];
wchar_t sz_D12[64];
wchar_t sz_D13[64];
wchar_t sz_D14[64];
wchar_t sz_D15[64];
wchar_t sz_D16[64];
wchar_t sz_D17[64];
wchar_t sz_D18[64];
wchar_t sz_D19[64];
wchar_t sz_D20[64];
wchar_t sz_D21[64];
wchar_t sz_D22[64];
wchar_t sz_D23[64];
wchar_t sz_D24[64];
wchar_t sz_D25[64];
wchar_t sz_D26[64];
wchar_t sz_D27[64];
wchar_t sz_D28[64];
wchar_t sz_E1[64];
wchar_t sz_E2[64];
wchar_t sz_F1[64];
wchar_t sz_F3[64];
wchar_t sz_F4[64];
wchar_t sz_F5[128];
wchar_t sz_F6[64];
wchar_t sz_F7[128];
wchar_t sz_F8[128];
wchar_t sz_F10[64];
wchar_t sz_F11[64];
wchar_t sz_G1[64];
wchar_t sz_G2[64];
wchar_t sz_G3[64];
wchar_t sz_H1[64];
wchar_t sz_H2[64];
wchar_t sz_H3[128];
wchar_t sz_H4[64];
wchar_t sz_H5[64];
wchar_t sz_H6[64];
wchar_t sz_H7[64];
wchar_t sz_H8[64];
wchar_t sz_H9[64];
wchar_t sz_H10[64];
wchar_t sz_H11[64];
wchar_t sz_H12[64];
wchar_t sz_H13[64];
wchar_t sz_H14[64];
wchar_t sz_H15[64];
wchar_t sz_H16[64];
wchar_t sz_H17[64];
wchar_t sz_H18[64];
wchar_t sz_H19[64];
wchar_t sz_H20[64];
wchar_t sz_H21[64];
wchar_t sz_H22[64];
wchar_t sz_H23[64];
wchar_t sz_H24[64];
wchar_t sz_H25[64];
wchar_t sz_H26[64];
wchar_t sz_H27[64];
wchar_t sz_H28[64];
wchar_t sz_H29[64];
wchar_t sz_H30[64];
wchar_t sz_H31[64];
wchar_t sz_H32[64];
wchar_t sz_H33[64];
wchar_t sz_H34[64];
wchar_t sz_H35[64];
wchar_t sz_H36[64];
wchar_t sz_H37[64];
wchar_t sz_H38[128];
wchar_t sz_H39[64];
wchar_t sz_H40[64];
wchar_t sz_H41[64];
wchar_t sz_H42[64];
wchar_t sz_H43[128];
wchar_t sz_H44[64];
wchar_t sz_H45[64];
wchar_t sz_H46[128];
wchar_t sz_H47[64];
wchar_t sz_H48[64];
wchar_t sz_H49[64];
wchar_t sz_H50[64];
wchar_t sz_H51[64];
wchar_t sz_H52[64];
wchar_t sz_H53[64];
wchar_t sz_H54[64];
wchar_t sz_H55[64];
wchar_t sz_H56[64];
wchar_t sz_H57[64];

wchar_t sz_H58[64];
wchar_t sz_H59[64];
wchar_t sz_H60[64];
wchar_t sz_H61[64]; 
wchar_t sz_H62[128];
wchar_t sz_H63[64];
wchar_t sz_H64[64];
wchar_t sz_H65[64];
wchar_t sz_H66[64];
wchar_t sz_H67[64];
wchar_t sz_H68[128];
wchar_t sz_H69[64];
wchar_t sz_H70[64];
wchar_t sz_H71[64];
wchar_t sz_H72[128];
wchar_t sz_H73[64];

wchar_t sz_I1[64];
wchar_t sz_I2[64];
wchar_t sz_I3[64];
wchar_t sz_I4[64];
wchar_t sz_J1[128];
wchar_t sz_J2[64];
wchar_t sz_K1[64];
wchar_t sz_K2[64];
wchar_t sz_K3[128];
wchar_t sz_K4[64];
wchar_t sz_K5[64];
wchar_t sz_K6[64];
wchar_t sz_K7[64];
wchar_t sz_L1[64];
wchar_t sz_L2[64];
wchar_t sz_L3[64];
wchar_t sz_L4[64];
wchar_t sz_L5[64];
wchar_t sz_L6[64];
wchar_t sz_L7[64];
wchar_t sz_L8[64];
wchar_t sz_L9[64];
wchar_t sz_L10[64];
wchar_t sz_L11[64];
wchar_t sz_L12[64];
wchar_t sz_L13[64];
wchar_t sz_L14[64];
wchar_t sz_L15[64];
wchar_t sz_L16[64];
wchar_t sz_L17[64];
wchar_t sz_L18[64];
wchar_t sz_L19[64];
wchar_t sz_L20[64];
wchar_t sz_L21[64];
wchar_t sz_L22[64];
wchar_t sz_L23[64];
wchar_t sz_L24[64];
wchar_t sz_L25[64];
wchar_t sz_L26[64];
wchar_t sz_L27[64];
wchar_t sz_L28[64];
wchar_t sz_L29[64];
wchar_t sz_L30[64];
wchar_t sz_L31[64];
wchar_t sz_L32[64];
wchar_t sz_L33[64];
wchar_t sz_L34[64];
wchar_t sz_L35[64];
wchar_t sz_L36[64];
wchar_t sz_L37[64];
wchar_t sz_L38[64];
wchar_t sz_L39[64];
wchar_t sz_L40[64];
wchar_t sz_L41[64];
wchar_t sz_L42[64];
wchar_t sz_L43[64];
wchar_t sz_L44[64];
wchar_t sz_L45[64];
wchar_t sz_L46[64];
wchar_t sz_L47[64];
wchar_t sz_L48[64];
wchar_t sz_L49[64];
wchar_t sz_L50[64];
wchar_t sz_L51[128];
wchar_t sz_L52[64];
wchar_t sz_L53[64];
wchar_t sz_L54[64];
wchar_t sz_L55[64];
wchar_t sz_L56[64];
wchar_t sz_L57[64];
wchar_t sz_L58[64];
wchar_t sz_L59[64];
wchar_t sz_L60[64];
wchar_t sz_L61[64];
wchar_t sz_L62[64];
wchar_t sz_L63[64];
wchar_t sz_L64[64];
wchar_t sz_L65[64];
wchar_t sz_L66[64];
wchar_t sz_L67[64];
wchar_t sz_L68[64];
wchar_t sz_L69[64];
wchar_t sz_L70[64];
wchar_t sz_L71[64];
wchar_t sz_L72[64];
wchar_t sz_L73[64];
wchar_t sz_L74[64];
wchar_t sz_L75[64];
wchar_t sz_L76[64];
wchar_t sz_L77[128];
wchar_t sz_L78[64];
wchar_t sz_L79[64];
wchar_t sz_L80[64];
wchar_t sz_L81[128];
wchar_t sz_L82[64];
wchar_t sz_L83[64];
wchar_t sz_L84[64];
wchar_t sz_L85[64];
wchar_t sz_L86[64];
wchar_t sz_L87[64];
wchar_t sz_L88[64];
wchar_t sz_L89[64];
wchar_t sz_L90[64];
wchar_t sz_L91[64];
wchar_t sz_L92[64];
wchar_t sz_L93[64];
wchar_t sz_L94[64];
TCHAR sz_ShowOptions[64];
TCHAR sz_HideOptions[64];
TCHAR sz_Computer[64];
TCHAR sz_ID[64];
TCHAR sz_Port[64];

// 14 April 2008 jdp
wchar_t sz_H94[64];
wchar_t sz_H95[64];
wchar_t sz_H96[64];
wchar_t sz_H97[64];
wchar_t sz_H98[64];
wchar_t sz_H99[64];
wchar_t sz_H100[64];
wchar_t sz_H101[64];
wchar_t sz_H102[128];
// File/dir Rename messages
wchar_t sz_M1[64];
wchar_t sz_M2[64];
wchar_t sz_M3[64];
wchar_t sz_M4[64];
wchar_t sz_M5[64];
wchar_t sz_M6[64];
wchar_t sz_M7[64];
wchar_t sz_M8[64];
wchar_t sz_N1[64];
wchar_t sz_N2[64];
wchar_t sz_N3[64];
wchar_t sz_N4[64];
wchar_t sz_N5[64];
wchar_t sz_N6[64];
wchar_t sz_N7[64];
wchar_t sz_N8[64];
wchar_t sz_N9[64];
wchar_t sz_N10[64];
wchar_t sz_N11[64];
wchar_t sz_N12[64];
wchar_t sz_N13[64];
wchar_t sz_N14[64];


bool command_line=true;
bool g_passwordfailed=true;
bool g_ConnectionLossAlreadyReported = false;


// Accelerator Keys
AccelKeys TheAccelKeys;
HINSTANCE m_hInstResDLL;
HINSTANCE hInstance;


typedef void (CALLBACK* LPFNSETDLLDIRECTORY)(LPCTSTR);
static LPFNSETDLLDIRECTORY MySetDllDirectory = NULL;

static BOOL read_reg_string(HKEY key, LPCTSTR sub_key, LPCTSTR val_name, LPBYTE data, LPDWORD data_len) {
        HKEY hkey;
        BOOL ret = FALSE;
        int retv;

        if(ERROR_SUCCESS == RegOpenKeyEx(key, 
                                         sub_key, 
					 0,  KEY_QUERY_VALUE, &hkey)) {
                if(ERROR_SUCCESS == (retv=RegQueryValueEx(hkey, val_name, 0, NULL, data, data_len)))
                        ret = TRUE;
                else
                        vnclog.Print(3,_T("Could not read reg key '%s' subkey '%s' value: '%s'\nError: %u\n"),
                               ((key == HKEY_LOCAL_MACHINE) ? _T("HKLM") : (key == HKEY_CURRENT_USER) ? _T("HKCU") : _T("???")),
                               sub_key, val_name, (UINT)GetLastError());
                RegCloseKey(key);
        }
        else
                vnclog.Print(3,_T("Could not open reg subkey: %s\nError: %u\n"), sub_key, (UINT)GetLastError());

        return ret;
}

//#define CRASHRPT
#ifdef CRASHRPT
#ifndef _X64
#include "C:/DATA/crash/crashrpt/include/crashrpt.h"
#pragma comment(lib, "C:/DATA/crash/crashrpt/lib/CrashRpt1403")
#else
#include "C:/DATA/crash/crashrpt/include/crashrpt.h"
#pragma comment(lib, "C:/DATA/crash/crashrpt/lib/x64/CrashRpt1403")
#endif
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR szCmdLine, int nCmdShow)
{
	HMODULE hUser32 = LoadLibraryEx(_T("user32.dll"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	HMODULE shcoreDLL = LoadLibraryExW(_T("SHCORE.DLL"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	//Min  Vista
	typedef BOOL(WINAPI *SetProcessDPIAwareFunc)();
	SetProcessDPIAwareFunc setDPIAwareF = NULL;
	//Min Windows 8.1
	typedef HRESULT(WINAPI *SetProcessDpiAwarenessFunc) (PROCESS_DPI_AWARENESS);
	SetProcessDpiAwarenessFunc setDPIpiAwarenessF = NULL;
	//Min Windows 10, version 1703
	typedef HRESULT(WINAPI *SetProcessDpiAwarenessContextFunc) (DPI_AWARENESS_CONTEXT);
	SetProcessDpiAwarenessContextFunc SetProcessDpiAwarenessContextF = NULL;
	if (hUser32) {
		setDPIAwareF = (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
		SetProcessDpiAwarenessContextF = (SetProcessDpiAwarenessContextFunc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
	}
	if (shcoreDLL) {
		setDPIpiAwarenessF =  (SetProcessDpiAwarenessFunc)GetProcAddress(shcoreDLL, "SetProcessDpiAwareness");
	}

	HRESULT hr = S_FALSE;
	if (SetProcessDpiAwarenessContextF) 
		hr = SetProcessDpiAwarenessContextF(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	if (hr != S_OK && setDPIpiAwarenessF)
		hr = setDPIpiAwarenessF(PROCESS_PER_MONITOR_DPI_AWARE);
	if (hr != S_OK && (setDPIAwareF))
		setDPIAwareF();

	if (hUser32) 
		FreeLibrary(hUser32);
	if (shcoreDLL) 
		FreeLibrary(shcoreDLL);

#ifdef CRASHRPT
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);
	info.pszAppName = _T("UVNC");
	info.pszAppVersion = _T("1.4.4.0-dev");
	info.pszEmailSubject = _T("UltraVNC Viewer 1.4.4.0-dev Error Report");
	info.pszEmailTo = _T("uvnc@skynet.be");
	info.uPriorities[CR_SMAPI] = 1; // Third try send report over Simple MAPI    
	// Install all available exception handlers
	info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;
	// Restart the app on crash
	info.dwFlags |= CR_INST_APP_RESTART;
	info.dwFlags |= CR_INST_SEND_QUEUED_REPORTS;
	info.dwFlags |= CR_INST_AUTO_THREAD_HANDLERS;
	info.pszRestartCmdLine = _T("/restart");
	// Define the Privacy Policy URL

	// Install crash reporting
	int nResult = crInstall(&info);
	if (nResult != 0)
	{
		// Something goes wrong. Get error message.
		TCHAR szErrorMsg[512] = _T("");
		crGetLastErrorMsg(szErrorMsg, 512);
		_tprintf_s(_T("%s\n"), szErrorMsg);
		return 1;
	}
#endif

  setbuf(stderr, 0);
  bool console = false;
  ::hInstance = hInstance;
  m_hInstResDLL = NULL;

  // [v1.0.2-jp1 fix]
  //m_hInstResDLL = LoadLibrary("lang.dll");
  HMODULE hmod;
  HKEY hkey;
  if((hmod=GetModuleHandleW(_T("kernel32.dll"))) ) 
  {
	MySetDllDirectory = (LPFNSETDLLDIRECTORY)GetProcAddress(hmod, "SetDllDirectoryW");
	if(MySetDllDirectory)  MySetDllDirectory(_T(""));
	else
	{
		OSVERSIONINFO osinfo;
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&osinfo);
		if((osinfo.dwMajorVersion == 5 && osinfo.dwMinorVersion == 0 && _tcsicmp((TCHAR*)osinfo.szCSDVersion, _T("Service Pack 3")) >= 0) ||
                   (osinfo.dwMajorVersion == 5 &&  osinfo.dwMinorVersion == 1 && _tcsicmp((TCHAR*)osinfo.szCSDVersion, _T("")) >= 0)) 
		{
			DWORD regval = 1;
                        DWORD reglen = sizeof(DWORD);

                        vnclog.Print(3,_T("Using Win2k (SP3+) / WinXP (No SP).. Checking SafeDllSearch\n"));
                        read_reg_string(HKEY_LOCAL_MACHINE,
                                        _T("System\\CurrentControlSet\\Control\\Session Manager"),
                                        _T("SafeDllSearchMode"),
                                        (LPBYTE)&regval,
                                        &reglen);

                        if(regval != 0) {
                                vnclog.Print(3,_T("Trying to set SafeDllSearchMode to 0\n"));
                                regval = 0;
                                if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                                _T("System\\CurrentControlSet\\Control\\Session Manager"), 
                                                0,  KEY_SET_VALUE, &hkey) == ERROR_SUCCESS) {
                                        if(RegSetValueEx(hkey, 
                                                         _T("SafeDllSearchMode"),
                                                         0,
                                                         REG_DWORD,
                                                         (LPBYTE) &regval,
                                                         sizeof(DWORD)) != ERROR_SUCCESS)
                                                vnclog.Print(3,_T("Error writing SafeDllSearchMode. Error: %u\n"),(UINT)GetLastError());
                                        RegCloseKey(hkey);
                                }
                                else
                                        vnclog.Print(3,_T("Error opening Session Manager key for writing. Error: %u\n"),(UINT)GetLastError());
                        }
                        else
                                vnclog.Print(3,_T("SafeDllSearchMode is set to 0\n"));


		}


	}
  }

  //limit the vnclang.dll searchpath to avoid
  wchar_t szCurrentDir[MAX_PATH]=L"";
  wchar_t szCurrentDir_vnclangdll[MAX_PATH]=L"";
  if (GetModuleFileNameW(NULL, szCurrentDir, MAX_PATH))
	{
		wchar_t* p = wcsrchr(szCurrentDir, L'\\');
		*p = L'\0';
	}
	
  // Load saved language preference from options.vnc
  wchar_t optionsFile[MAX_PATH];
  wchar_t savedLanguage[32] = L"en";  // Default to English
  
  // Try to find and load options file
  wcscpy_s(optionsFile, _countof(optionsFile), szCurrentDir);
  wcscat_s(optionsFile, _countof(optionsFile), L"\\options.vnc");
  
  // Check if file exists and read language setting
  HANDLE hFile = CreateFileW(optionsFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
	  // Try AppData folder
	  if (SHGetFolderPathW(0, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, optionsFile) == S_OK) {
		  wcscat_s(optionsFile, _countof(optionsFile), L"\\UltraVNC\\");
		  wcscat_s(optionsFile, _countof(optionsFile), L"options.vnc");
		  hFile = CreateFileW(optionsFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	  }
  }
  
  if (hFile != INVALID_HANDLE_VALUE) {
	  CloseHandle(hFile);
	  // Read language from INI file using GetPrivateProfileString
	  GetPrivateProfileStringW(L"options", L"language", L"en", savedLanguage, _countof(savedLanguage), optionsFile);
  }
  
  // Load appropriate language DLL based on saved preference
  m_hInstResDLL = hInstance;  // Default to English
  
  if (_wcsicmp(savedLanguage, L"en") != 0) {
	  // Try to load language DLL
	  swprintf_s(szCurrentDir_vnclangdll, _countof(szCurrentDir_vnclangdll), L"%s\\languages\\vnclang_%s.dll", szCurrentDir, savedLanguage);
	  HMODULE hLangDLL = LoadLibraryW(szCurrentDir_vnclangdll);
	  
	  if (hLangDLL) {
		  m_hInstResDLL = hLangDLL;
	  } else {
		  // Fallback: try root folder
		  swprintf_s(szCurrentDir_vnclangdll, _countof(szCurrentDir_vnclangdll), L"%s\\vnclang_%s.dll", szCurrentDir, savedLanguage);
		  hLangDLL = LoadLibraryW(szCurrentDir_vnclangdll);
		  if (hLangDLL) {
			  m_hInstResDLL = hLangDLL;
		  }
	  }
  }
  
  if (_tcscmp(szCmdLine,_T(""))==0) command_line=false;
  LoadStringW(m_hInstResDLL, IDS_A1, sz_A1, _countof(sz_A1));
  LoadStringW(m_hInstResDLL, IDS_A2, sz_A2, _countof(sz_A2));
  LoadStringW(m_hInstResDLL, IDS_A3, sz_A3, _countof(sz_A3));
  LoadStringW(m_hInstResDLL, IDS_A4, sz_A4, _countof(sz_A4));
  LoadStringW(m_hInstResDLL, IDS_A5, sz_A5, _countof(sz_A5));
  LoadStringW(m_hInstResDLL, IDS_B1, sz_B1, _countof(sz_B1));
  LoadStringW(m_hInstResDLL, IDS_B2, sz_B2, _countof(sz_B2));
  LoadStringW(m_hInstResDLL, IDS_B3, sz_B3, _countof(sz_B3));
  LoadStringW(m_hInstResDLL, IDS_C1, sz_C1, _countof(sz_C1));
  LoadStringW(m_hInstResDLL, IDS_C2, sz_C2, _countof(sz_C2));
  LoadStringW(m_hInstResDLL, IDS_C3, sz_C3, _countof(sz_C3));
  LoadStringW(m_hInstResDLL, IDS_D1, sz_D1, _countof(sz_D1));
  LoadStringW(m_hInstResDLL, IDS_D2, sz_D2, _countof(sz_D2));
  LoadStringW(m_hInstResDLL, IDS_D3, sz_D3, _countof(sz_D3));
  LoadStringW(m_hInstResDLL, IDS_D4, sz_D4, _countof(sz_D4));
  LoadStringW(m_hInstResDLL, IDS_D5, sz_D5, _countof(sz_D5));
  LoadStringW(m_hInstResDLL, IDS_D6, sz_D6, _countof(sz_D6));
  LoadStringW(m_hInstResDLL, IDS_D7, sz_D7, _countof(sz_D7));
  LoadStringW(m_hInstResDLL, IDS_D8, sz_D8, _countof(sz_D8));
  LoadStringW(m_hInstResDLL, IDS_D9, sz_D9, _countof(sz_D9));
  LoadStringW(m_hInstResDLL, IDS_D10, sz_D10, _countof(sz_D10));
  LoadStringW(m_hInstResDLL, IDS_D11, sz_D11, _countof(sz_D11));
  LoadStringW(m_hInstResDLL, IDS_D12, sz_D12, _countof(sz_D12));
  LoadStringW(m_hInstResDLL, IDS_D13, sz_D13, _countof(sz_D13));
  LoadStringW(m_hInstResDLL, IDS_D14, sz_D14, _countof(sz_D14));
  LoadStringW(m_hInstResDLL, IDS_D15, sz_D15, _countof(sz_D15));
  LoadStringW(m_hInstResDLL, IDS_D16, sz_D16, _countof(sz_D16));
  LoadStringW(m_hInstResDLL, IDS_D17, sz_D17, _countof(sz_D17));
  LoadStringW(m_hInstResDLL, IDS_D18, sz_D18, _countof(sz_D18));
  LoadStringW(m_hInstResDLL, IDS_D19, sz_D19, _countof(sz_D19));
  LoadStringW(m_hInstResDLL, IDS_D20, sz_D20, _countof(sz_D20));
  LoadStringW(m_hInstResDLL, IDS_D21, sz_D21, _countof(sz_D21));
  LoadStringW(m_hInstResDLL, IDS_D22, sz_D22, _countof(sz_D22));
  LoadStringW(m_hInstResDLL, IDS_D23, sz_D23, _countof(sz_D23));
  LoadStringW(m_hInstResDLL, IDS_D24, sz_D24, _countof(sz_D24));
  LoadStringW(m_hInstResDLL, IDS_D25, sz_D25, _countof(sz_D25));
  LoadStringW(m_hInstResDLL, IDS_D26, sz_D26, _countof(sz_D26));
  LoadStringW(m_hInstResDLL, IDS_D27, sz_D27, _countof(sz_D27));
  LoadStringW(m_hInstResDLL, IDS_D28, sz_D28, _countof(sz_D28));
  LoadStringW(m_hInstResDLL, IDS_E1, sz_E1, _countof(sz_E1));
  LoadStringW(m_hInstResDLL, IDS_E2, sz_E2, _countof(sz_E2));
  LoadStringW(m_hInstResDLL, IDS_F1, sz_F1, _countof(sz_F1));
  LoadStringW(m_hInstResDLL, IDS_F3, sz_F3, _countof(sz_F3));
  LoadStringW(m_hInstResDLL, IDS_F4, sz_F4, _countof(sz_F4));
  LoadStringW(m_hInstResDLL, IDS_F5, sz_F5, _countof(sz_F5));
  LoadStringW(m_hInstResDLL, IDS_F6, sz_F6, _countof(sz_F6));
  LoadStringW(m_hInstResDLL, IDS_F7, sz_F7, _countof(sz_F7));
  LoadStringW(m_hInstResDLL, IDS_F8, sz_F8, _countof(sz_F8));
  LoadStringW(m_hInstResDLL, IDS_F10, sz_F10, _countof(sz_F10));
  LoadStringW(m_hInstResDLL, IDS_F11, sz_F11, _countof(sz_F11));
  LoadStringW(m_hInstResDLL, IDS_G1, sz_G1, _countof(sz_G1));
  LoadStringW(m_hInstResDLL, IDS_G1, sz_G2, _countof(sz_G2));
  LoadStringW(m_hInstResDLL, IDS_G1, sz_G3, _countof(sz_G3));

  LoadStringW(m_hInstResDLL, IDS_H1, sz_H1, _countof(sz_H1));
  LoadStringW(m_hInstResDLL, IDS_H2, sz_H2, _countof(sz_H2));
  LoadStringW(m_hInstResDLL, IDS_H3, sz_H3, _countof(sz_H3));
  LoadStringW(m_hInstResDLL, IDS_H4, sz_H4, _countof(sz_H4));
  LoadStringW(m_hInstResDLL, IDS_H5, sz_H5, _countof(sz_H5));
  LoadStringW(m_hInstResDLL, IDS_H6, sz_H6, _countof(sz_H6));
  LoadStringW(m_hInstResDLL, IDS_H7, sz_H7, _countof(sz_H7));
  LoadStringW(m_hInstResDLL, IDS_H8, sz_H8, _countof(sz_H8));
  LoadStringW(m_hInstResDLL, IDS_H9, sz_H9, _countof(sz_H9));
  LoadStringW(m_hInstResDLL, IDS_H10, sz_H10, _countof(sz_H10));
  LoadStringW(m_hInstResDLL, IDS_H11, sz_H11, _countof(sz_H11));
  LoadStringW(m_hInstResDLL, IDS_H12, sz_H12, _countof(sz_H12));
  LoadStringW(m_hInstResDLL, IDS_H13, sz_H13, _countof(sz_H13));
  LoadStringW(m_hInstResDLL, IDS_H14, sz_H14, _countof(sz_H14));
  LoadStringW(m_hInstResDLL, IDS_H15, sz_H15, _countof(sz_H15));
  LoadStringW(m_hInstResDLL, IDS_H16, sz_H16, _countof(sz_H16));
  LoadStringW(m_hInstResDLL, IDS_H17, sz_H17, _countof(sz_H17));
  LoadStringW(m_hInstResDLL, IDS_H18, sz_H18, _countof(sz_H18));
  LoadStringW(m_hInstResDLL, IDS_H19, sz_H19, _countof(sz_H19));
  LoadStringW(m_hInstResDLL, IDS_H20, sz_H20, _countof(sz_H20));
  LoadStringW(m_hInstResDLL, IDS_H21, sz_H21, _countof(sz_H21));
  LoadStringW(m_hInstResDLL, IDS_H22, sz_H22, _countof(sz_H22));
  LoadStringW(m_hInstResDLL, IDS_H23, sz_H23, _countof(sz_H23));
  LoadStringW(m_hInstResDLL, IDS_H24, sz_H24, _countof(sz_H24));
  LoadStringW(m_hInstResDLL, IDS_H25, sz_H25, _countof(sz_H25));
  LoadStringW(m_hInstResDLL, IDS_H26, sz_H26, _countof(sz_H26));
  LoadStringW(m_hInstResDLL, IDS_H27, sz_H27, _countof(sz_H27));
  LoadStringW(m_hInstResDLL, IDS_H28, sz_H28, _countof(sz_H28));
  LoadStringW(m_hInstResDLL, IDS_H29, sz_H29, _countof(sz_H29));
  LoadStringW(m_hInstResDLL, IDS_H30, sz_H30, _countof(sz_H30));
  LoadStringW(m_hInstResDLL, IDS_H31, sz_H31, _countof(sz_H31));
  LoadStringW(m_hInstResDLL, IDS_H32, sz_H32, _countof(sz_H32));
  LoadStringW(m_hInstResDLL, IDS_H33, sz_H33, _countof(sz_H33));
  LoadStringW(m_hInstResDLL, IDS_H34, sz_H34, _countof(sz_H34));
  LoadStringW(m_hInstResDLL, IDS_H35, sz_H35, _countof(sz_H35));
  LoadStringW(m_hInstResDLL, IDS_H36, sz_H36, _countof(sz_H36));
  LoadStringW(m_hInstResDLL, IDS_H37, sz_H37, _countof(sz_H37));
  LoadStringW(m_hInstResDLL, IDS_H38, sz_H38, _countof(sz_H38));
  LoadStringW(m_hInstResDLL, IDS_H39, sz_H39, _countof(sz_H39));
  LoadStringW(m_hInstResDLL, IDS_H40, sz_H40, _countof(sz_H40));
  LoadStringW(m_hInstResDLL, IDS_H41, sz_H41, _countof(sz_H41));
  LoadStringW(m_hInstResDLL, IDS_H42, sz_H42, _countof(sz_H42));
  LoadStringW(m_hInstResDLL, IDS_H43, sz_H43, _countof(sz_H43));
  LoadStringW(m_hInstResDLL, IDS_H44, sz_H44, _countof(sz_H44));
  LoadStringW(m_hInstResDLL, IDS_H45, sz_H45, _countof(sz_H45));
  LoadStringW(m_hInstResDLL, IDS_H46, sz_H46, _countof(sz_H46));
  LoadStringW(m_hInstResDLL, IDS_H47, sz_H47, _countof(sz_H47));
  LoadStringW(m_hInstResDLL, IDS_H48, sz_H48, _countof(sz_H48));
  LoadStringW(m_hInstResDLL, IDS_H49, sz_H49, _countof(sz_H49));
  LoadStringW(m_hInstResDLL, IDS_H50, sz_H50, _countof(sz_H50));
  LoadStringW(m_hInstResDLL, IDS_H51, sz_H51, _countof(sz_H51));
  LoadStringW(m_hInstResDLL, IDS_H52, sz_H52, _countof(sz_H52));
  LoadStringW(m_hInstResDLL, IDS_H53, sz_H53, _countof(sz_H53));
  LoadStringW(m_hInstResDLL, IDS_H54, sz_H54, _countof(sz_H54));
  LoadStringW(m_hInstResDLL, IDS_H55, sz_H55, _countof(sz_H55));
  LoadStringW(m_hInstResDLL, IDS_H56, sz_H56, _countof(sz_H56));
  LoadStringW(m_hInstResDLL, IDS_H57, sz_H57, _countof(sz_H57));

  LoadStringW(m_hInstResDLL, IDS_H58, sz_H58, _countof(sz_H58));
  LoadStringW(m_hInstResDLL, IDS_H59, sz_H59, _countof(sz_H59));
  LoadStringW(m_hInstResDLL, IDS_H60, sz_H60, _countof(sz_H60));
  LoadStringW(m_hInstResDLL, IDS_H61, sz_H61, _countof(sz_H61));
  LoadStringW(m_hInstResDLL, IDS_H62, sz_H62, _countof(sz_H62));
  LoadStringW(m_hInstResDLL, IDS_H63, sz_H63, _countof(sz_H63));
  LoadStringW(m_hInstResDLL, IDS_H64, sz_H64, _countof(sz_H64));
  LoadStringW(m_hInstResDLL, IDS_H65, sz_H65, _countof(sz_H65));
  LoadStringW(m_hInstResDLL, IDS_H66, sz_H66, _countof(sz_H66));
  LoadStringW(m_hInstResDLL, IDS_H67, sz_H67, _countof(sz_H67));
  LoadStringW(m_hInstResDLL, IDS_H68, sz_H68, _countof(sz_H68));
  LoadStringW(m_hInstResDLL, IDS_H69, sz_H69, _countof(sz_H69));
  LoadStringW(m_hInstResDLL, IDS_H70, sz_H70, _countof(sz_H70));
  LoadStringW(m_hInstResDLL, IDS_H71, sz_H71, _countof(sz_H71));
  LoadStringW(m_hInstResDLL, IDS_H72, sz_H72, _countof(sz_H72));
  LoadStringW(m_hInstResDLL, IDS_H73, sz_H73, _countof(sz_H73));

  
	LoadStringW(m_hInstResDLL, IDS_I1, sz_I1, _countof(sz_I1));
	LoadStringW(m_hInstResDLL, IDS_I2, sz_I2, _countof(sz_I2));
	LoadStringW(m_hInstResDLL, IDS_I3, sz_I3, _countof(sz_I3));

	LoadStringW(m_hInstResDLL, IDS_J1, sz_J1, _countof(sz_J1));
	LoadStringW(m_hInstResDLL, IDS_J2, sz_J2, _countof(sz_J2));

	LoadStringW(m_hInstResDLL, IDS_K1, sz_K1, _countof(sz_K1));
	LoadStringW(m_hInstResDLL, IDS_K2, sz_K2, _countof(sz_K2));
	LoadStringW(m_hInstResDLL, IDS_K3, sz_K3, sizeof(sz_K3)/sizeof(sz_K3[0]));
	LoadStringW(m_hInstResDLL, IDS_K4, sz_K4, _countof(sz_K4));
	LoadStringW(m_hInstResDLL, IDS_K5, sz_K5, _countof(sz_K5));
	LoadStringW(m_hInstResDLL, IDS_K6, sz_K6, _countof(sz_K6));

	LoadStringW(m_hInstResDLL, IDS_L1, sz_L1, _countof(sz_L1));
	LoadStringW(m_hInstResDLL, IDS_L2, sz_L2, _countof(sz_L2));
	LoadStringW(m_hInstResDLL, IDS_L3, sz_L3, _countof(sz_L3));
	LoadStringW(m_hInstResDLL, IDS_L4, sz_L4, _countof(sz_L4));
	LoadStringW(m_hInstResDLL, IDS_L5, sz_L5, _countof(sz_L5));
	LoadStringW(m_hInstResDLL, IDS_L6, sz_L6, _countof(sz_L6));
	LoadStringW(m_hInstResDLL, IDS_L7, sz_L7, _countof(sz_L7));
	LoadStringW(m_hInstResDLL, IDS_L8, sz_L8, _countof(sz_L8));
	LoadStringW(m_hInstResDLL, IDS_L9, sz_L9, _countof(sz_L9));
	LoadStringW(m_hInstResDLL, IDS_L10, sz_L10, _countof(sz_L10));
	LoadStringW(m_hInstResDLL, IDS_L11, sz_L11, _countof(sz_L11));
	LoadStringW(m_hInstResDLL, IDS_L12, sz_L12, _countof(sz_L12));
	LoadStringW(m_hInstResDLL, IDS_L13, sz_L13, _countof(sz_L13));
	LoadStringW(m_hInstResDLL, IDS_L14, sz_L14, _countof(sz_L14));
	LoadStringW(m_hInstResDLL, IDS_L15, sz_L15, _countof(sz_L15));
	LoadStringW(m_hInstResDLL, IDS_L16, sz_L16, _countof(sz_L16));
	LoadStringW(m_hInstResDLL, IDS_L17, sz_L17, _countof(sz_L17));
	LoadStringW(m_hInstResDLL, IDS_L18, sz_L18, _countof(sz_L18));
	LoadStringW(m_hInstResDLL, IDS_L19, sz_L19, _countof(sz_L19));
	LoadStringW(m_hInstResDLL, IDS_L20, sz_L20, _countof(sz_L20));
	LoadStringW(m_hInstResDLL, IDS_L21, sz_L21, _countof(sz_L21));
	LoadStringW(m_hInstResDLL, IDS_L22, sz_L22, _countof(sz_L22));
	LoadStringW(m_hInstResDLL, IDS_L23, sz_L23, _countof(sz_L23));
	LoadStringW(m_hInstResDLL, IDS_L24, sz_L24, _countof(sz_L24));
	LoadStringW(m_hInstResDLL, IDS_L25, sz_L25, _countof(sz_L25));
	LoadStringW(m_hInstResDLL, IDS_L26, sz_L26, _countof(sz_L26));
	LoadStringW(m_hInstResDLL, IDS_L27, sz_L27, _countof(sz_L27));
	LoadStringW(m_hInstResDLL, IDS_L28, sz_L28, _countof(sz_L28));
	LoadStringW(m_hInstResDLL, IDS_L29, sz_L29, _countof(sz_L29));
	LoadStringW(m_hInstResDLL, IDS_L30, sz_L30, _countof(sz_L30));
	LoadStringW(m_hInstResDLL, IDS_L31, sz_L31, _countof(sz_L31));
	LoadStringW(m_hInstResDLL, IDS_L32, sz_L32, _countof(sz_L32));
	LoadStringW(m_hInstResDLL, IDS_L33, sz_L33, _countof(sz_L33));
	LoadStringW(m_hInstResDLL, IDS_L34, sz_L34, _countof(sz_L34));
	LoadStringW(m_hInstResDLL, IDS_L35, sz_L35, _countof(sz_L35));
	LoadStringW(m_hInstResDLL, IDS_L36, sz_L36, _countof(sz_L36));
	LoadStringW(m_hInstResDLL, IDS_L37, sz_L37, _countof(sz_L37));
	LoadStringW(m_hInstResDLL, IDS_L38, sz_L38, _countof(sz_L38));
	LoadStringW(m_hInstResDLL, IDS_L39, sz_L39, _countof(sz_L39));
	LoadStringW(m_hInstResDLL, IDS_L40, sz_L40, _countof(sz_L40));
	LoadStringW(m_hInstResDLL, IDS_L41, sz_L41, _countof(sz_L41));
	LoadStringW(m_hInstResDLL, IDS_L42, sz_L42, _countof(sz_L42));
	LoadStringW(m_hInstResDLL, IDS_L43, sz_L43, _countof(sz_L43));
	LoadStringW(m_hInstResDLL, IDS_L44, sz_L44, _countof(sz_L44));
	LoadStringW(m_hInstResDLL, IDS_L45, sz_L45, _countof(sz_L45));
	LoadStringW(m_hInstResDLL, IDS_L46, sz_L46, _countof(sz_L46));
	LoadStringW(m_hInstResDLL, IDS_L47, sz_L47, _countof(sz_L47));
	LoadStringW(m_hInstResDLL, IDS_L48, sz_L48, _countof(sz_L48));
	LoadStringW(m_hInstResDLL, IDS_L49, sz_L49, _countof(sz_L49));
	LoadStringW(m_hInstResDLL, IDS_L50, sz_L50, _countof(sz_L50));
	LoadStringW(m_hInstResDLL, IDS_L51, sz_L51, _countof(sz_L51));
	LoadStringW(m_hInstResDLL, IDS_L52, sz_L52, _countof(sz_L52));
	LoadStringW(m_hInstResDLL, IDS_L53, sz_L53, _countof(sz_L53));
	LoadStringW(m_hInstResDLL, IDS_L54, sz_L54, _countof(sz_L54));
	LoadStringW(m_hInstResDLL, IDS_L55, sz_L55, _countof(sz_L55));
	LoadStringW(m_hInstResDLL, IDS_L56, sz_L56, _countof(sz_L56));
	LoadStringW(m_hInstResDLL, IDS_L57, sz_L57, _countof(sz_L57));
	LoadStringW(m_hInstResDLL, IDS_L58, sz_L58, _countof(sz_L58));
	LoadStringW(m_hInstResDLL, IDS_L59, sz_L59, _countof(sz_L59));
	LoadStringW(m_hInstResDLL, IDS_L60, sz_L60, _countof(sz_L60));
	LoadStringW(m_hInstResDLL, IDS_L61, sz_L61, _countof(sz_L61));
	LoadStringW(m_hInstResDLL, IDS_L62, sz_L62, _countof(sz_L62));
	LoadStringW(m_hInstResDLL, IDS_L63, sz_L63, _countof(sz_L63));
	LoadStringW(m_hInstResDLL, IDS_L64, sz_L64, _countof(sz_L64));
	LoadStringW(m_hInstResDLL, IDS_L65, sz_L65, _countof(sz_L65));
	LoadStringW(m_hInstResDLL, IDS_L66, sz_L66, _countof(sz_L66));
	LoadStringW(m_hInstResDLL, IDS_L67, sz_L67, _countof(sz_L67));
	LoadStringW(m_hInstResDLL, IDS_L68, sz_L68, _countof(sz_L68));
	LoadStringW(m_hInstResDLL, IDS_L69, sz_L69, _countof(sz_L69));
	LoadStringW(m_hInstResDLL, IDS_L70, sz_L70, _countof(sz_L70));
	LoadStringW(m_hInstResDLL, IDS_L71, sz_L71, _countof(sz_L71));
	LoadStringW(m_hInstResDLL, IDS_L72, sz_L72, _countof(sz_L72));
	LoadStringW(m_hInstResDLL, IDS_L73, sz_L73, _countof(sz_L73));
	LoadStringW(m_hInstResDLL, IDS_L74, sz_L74, _countof(sz_L74));
	LoadStringW(m_hInstResDLL, IDS_L75, sz_L75, _countof(sz_L75));
	LoadStringW(m_hInstResDLL, IDS_L76, sz_L76, _countof(sz_L76));
	LoadStringW(m_hInstResDLL, IDS_L77, sz_L77, _countof(sz_L77));
	LoadStringW(m_hInstResDLL, IDS_L78, sz_L78, _countof(sz_L78));
	LoadStringW(m_hInstResDLL, IDS_L79, sz_L79, _countof(sz_L79));
	LoadStringW(m_hInstResDLL, IDS_L80, sz_L80, _countof(sz_L80));
	LoadStringW(m_hInstResDLL, IDS_L81, sz_L81, _countof(sz_L81));
	LoadStringW(m_hInstResDLL, IDS_L82, sz_L82, _countof(sz_L82));
	LoadStringW(m_hInstResDLL, IDS_L83, sz_L83, _countof(sz_L83));
	LoadStringW(m_hInstResDLL, IDS_L84, sz_L84, _countof(sz_L84));
	LoadStringW(m_hInstResDLL, IDS_L85, sz_L85, _countof(sz_L85));
	LoadStringW(m_hInstResDLL, IDS_L86, sz_L86, _countof(sz_L86));
	LoadStringW(m_hInstResDLL, IDS_L87, sz_L87, _countof(sz_L87));
	LoadStringW(m_hInstResDLL, IDS_L88, sz_L88, _countof(sz_L88));
	LoadStringW(m_hInstResDLL, IDS_L89, sz_L89, _countof(sz_L89));
	LoadStringW(m_hInstResDLL, IDS_L90, sz_L90, _countof(sz_L90));
	LoadStringW(m_hInstResDLL, IDS_L91, sz_L91, _countof(sz_L91));
	LoadStringW(m_hInstResDLL, IDS_L92, sz_L92, _countof(sz_L92));
	LoadStringW(m_hInstResDLL, IDS_L93, sz_L93, _countof(sz_L93));
	LoadStringW(m_hInstResDLL, IDS_L94, sz_L94, _countof(sz_L94));
	LoadStringW(m_hInstResDLL, IDS_L95, sz_ShowOptions, sizeof(sz_ShowOptions) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L96, sz_HideOptions, sizeof(sz_HideOptions) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L97, sz_Computer, sizeof(sz_Computer) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L98, sz_ID, sizeof(sz_ID) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L99, sz_Port, sizeof(sz_Port) / sizeof(TCHAR));

	LoadStringW(m_hInstResDLL, IDS_M1, sz_M1, _countof(sz_M1));
	LoadStringW(m_hInstResDLL, IDS_M2, sz_M2, _countof(sz_M2));
	LoadStringW(m_hInstResDLL, IDS_M3, sz_M3, _countof(sz_M3));
	LoadStringW(m_hInstResDLL, IDS_M4, sz_M4, _countof(sz_M4));
	LoadStringW(m_hInstResDLL, IDS_M5, sz_M5, _countof(sz_M5));
	LoadStringW(m_hInstResDLL, IDS_M6, sz_M6, _countof(sz_M6));
	LoadStringW(m_hInstResDLL, IDS_M7, sz_M7, _countof(sz_M7));
	LoadStringW(m_hInstResDLL, IDS_M8, sz_M8, _countof(sz_M8));  

    // 14 April 2008 jdp
	LoadStringW(m_hInstResDLL, IDS_H94, sz_H94, _countof(sz_H94));
	LoadStringW(m_hInstResDLL, IDS_H95, sz_H95, _countof(sz_H95));
	LoadStringW(m_hInstResDLL, IDS_H96, sz_H96, _countof(sz_H96));
	LoadStringW(m_hInstResDLL, IDS_H97, sz_H97, _countof(sz_H97));
	LoadStringW(m_hInstResDLL, IDS_H98, sz_H98, _countof(sz_H98));
	LoadStringW(m_hInstResDLL, IDS_H99, sz_H99, _countof(sz_H99));
    LoadStringW(m_hInstResDLL, IDS_H100, sz_H100, _countof(sz_H100));
    LoadStringW(m_hInstResDLL, IDS_H101, sz_H101, _countof(sz_H101));
    LoadStringW(m_hInstResDLL, IDS_H102, sz_H102, _countof(sz_H102));
//	RegisterLinkLabel(m_hInstResDLL);
	loadStrings(m_hInstResDLL);

	/////////////////////////////////////////////////////////////

	// The state of the application as a whole is contained in the one app object
		VNCviewerApp32 app(hInstance, szCmdLine);

    console = app.m_options.m_logToConsole;

	// Start a new connection if specified on command line, 
	// or if not in listening mode
	MSG msg;
	while(g_passwordfailed==true)
		{
			g_passwordfailed=false;
			if (app.m_options.m_connectionSpecified && !app.m_options.m_listening) {
				app.NewConnection(false,app.m_options.m_host_options, app.m_options.m_port);
			} else if (!app.m_options.m_listening) {
				// This one will also read from config file if specified
				app.NewConnection(false);
			}

			try
			{
					while ( GetMessage(&msg, NULL, 0,0) )
					{
						if (!TheAccelKeys.TranslateAccelKeys(&msg))
						{
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
					} 
			}
			catch (Exception &e)
			{
                if (!g_ConnectionLossAlreadyReported)
				e.Report();
			}
		}
		// Clean up winsock
	WSACleanup();	
	vnclog.Print(3, _T("Exiting\n"));
#ifdef CRASHRPT
	crUninstall();
#endif
    if (console) Sleep(2000);	
	return msg.wParam;
}


// Move the given window to the centre of the screen
// and bring it to the top.
void CentreWindow(HWND hwnd)
{
	RECT winrect, workrect;
	
	// Find how large the desktop work area is
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workrect, 0);
	//RECT workrect;
	/*tempdisplayclass tdc;
	tdc.Init();
	workrect.left=tdc.monarray[selected_screen].wl;
	workrect.right=tdc.monarray[selected_screen].wr;
	workrect.top=tdc.monarray[selected_screen].wt;
	workrect.bottom=tdc.monarray[selected_screen].wb;*/

	int workwidth = workrect.right -  workrect.left;
	int workheight = workrect.bottom - workrect.top;
	
	// And how big the window is
	GetWindowRect(hwnd, &winrect);
	int winwidth = winrect.right - winrect.left;
	int winheight = winrect.bottom - winrect.top;
	// Make sure it's not bigger than the work area
	winwidth = minimum(winwidth, workwidth);
	winheight = minimum(winheight, workheight);

	// Now centre it
	SetWindowPos(hwnd, 
		HWND_TOP,
		workrect.left + (workwidth-winwidth) / 2,
		workrect.top + (workheight-winheight) / 2,
		winwidth, winheight, 
		SWP_SHOWWINDOW);
	SetForegroundWindow(hwnd);
}


// sf@2002 - TightVNC method - RealVNC method
// Convert "host:display" or "host::port" or "host:port" if port > 100, into host and port
// IPv6 addresses must use bracket notation: [2001:db8::1]:5900
// Returns true if valid format, false if not.
// Takes initial string, addresses of results and size of host buffer in wchars.
// If the display info passed in is longer than the size of the host buffer, it
// is assumed to be invalid, so false is returned.
bool ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *pport) 
{
    if (hostlen < (int)_tcslen(display))
        return false;

    int tmp_port;
    TCHAR *colonpos = NULL;
    
    // Check for IPv6 bracket notation: [IPv6]:port
    if (display[0] == L'[') {
        TCHAR *bracketend = _tcschr(display, L']');
        if (bracketend == NULL)
            return false; // Invalid format - opening bracket without closing
        
        // Extract IPv6 address between brackets
        size_t ipv6len = bracketend - display - 1;
        if (ipv6len >= (size_t)hostlen)
            return false;
        
        _tcsncpy_s(phost, hostlen, display + 1, ipv6len);
        phost[ipv6len] = L'\0';
        
        // Check for port after the bracket
        if (bracketend[1] == L'\0') {
            // No port specified - use default
            tmp_port = RFB_PORT_OFFSET;
        }
        else if (bracketend[1] == L':') {
            if (bracketend[2] == L':') {
                // [IPv6]::port format
                if (_stscanf_s(bracketend + 3, TEXT("%d"), &tmp_port) != 1)
                    return false;
            }
            else {
                // [IPv6]:display or [IPv6]:port format
                if (_stscanf_s(bracketend + 2, TEXT("%d"), &tmp_port) != 1)
                    return false;
                
                // If port < 100 interpret as display number else as Port number
                if (tmp_port < 100)
                    tmp_port += RFB_PORT_OFFSET;
            }
        }
        else {
            return false; // Invalid format after bracket
        }
    }
    else {
        // IPv4 or hostname parsing (original logic)
        colonpos = _tcschr(display, L':');
        if (colonpos == NULL)
        {
            // No colon -- use default port number
            tmp_port = RFB_PORT_OFFSET;
            _tcsncpy_s(phost, MAX_HOST_NAME_LEN, display, MAX_HOST_NAME_LEN);
        }
        else
        {
            _tcsncpy_s(phost, MAX_HOST_NAME_LEN, display, colonpos - display);
            phost[colonpos - display] = L'\0';
            if (colonpos[1] == L':') {
                // Two colons -- interpret as a port number
                if (_stscanf_s(colonpos + 2, TEXT("%d"), &tmp_port) != 1) 
                    return false;
            }
            else
            {
                // One colon -- interpret as a display number or port number
                if (_stscanf_s(colonpos + 1, TEXT("%d"), &tmp_port) != 1) 
                    return false;

                // RealVNC method - If port < 100 interpret as display number else as Port number
                if (tmp_port < 100)
                    tmp_port += RFB_PORT_OFFSET;
            }
        }
    }
    *pport = tmp_port;
    return true;
}

// Reload language DLL and all strings
void ReloadLanguage(const wchar_t* langCode)
{
	extern HINSTANCE m_hInstResDLL;
	extern HINSTANCE hInstance;
	
	// Store old DLL handle
	HMODULE hOldDLL = m_hInstResDLL;
	
	// Build path to new language DLL
	wchar_t exePath[MAX_PATH];
	wchar_t langDllPath[MAX_PATH];
	
	if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
		wchar_t* lastSlash = wcsrchr(exePath, L'\\');
		if (lastSlash) {
			*lastSlash = L'\0';
			
			// Check if English (use embedded resources)
			if (_wcsicmp(langCode, L"en") == 0) {
				m_hInstResDLL = hInstance;
			}
			else {
				// Try to load from languages subfolder first
				swprintf_s(langDllPath, _countof(langDllPath), L"%s\\languages\\vnclang_%s.dll", exePath, langCode);
				HMODULE hNewDLL = LoadLibraryW(langDllPath);
				
				if (hNewDLL) {
					m_hInstResDLL = hNewDLL;
				}
				else {
					// Fallback to root folder
					swprintf_s(langDllPath, _countof(langDllPath), L"%s\\vnclang_%s.dll", exePath, langCode);
					hNewDLL = LoadLibraryW(langDllPath);
					
					if (hNewDLL) {
						m_hInstResDLL = hNewDLL;
					}
					else {
						// Fallback to English
						m_hInstResDLL = hInstance;
					}
				}
			}
		}
	}
	
	// Update m_hInstResDLL to point to the new DLL
	if (pApp) {
		m_hInstResDLL = m_hInstResDLL;
	}
	
	// Unload old DLL if it was a language DLL (not the main instance)
	if (hOldDLL != hInstance && hOldDLL != m_hInstResDLL) {
		FreeLibrary(hOldDLL);
	}
	
	// Reload all string resources
	LoadStringW(m_hInstResDLL, IDS_A1, sz_A1, _countof(sz_A1));
	LoadStringW(m_hInstResDLL, IDS_A2, sz_A2, _countof(sz_A2));
	LoadStringW(m_hInstResDLL, IDS_A3, sz_A3, _countof(sz_A3));
	LoadStringW(m_hInstResDLL, IDS_A4, sz_A4, _countof(sz_A4));
	LoadStringW(m_hInstResDLL, IDS_A5, sz_A5, _countof(sz_A5));
	LoadStringW(m_hInstResDLL, IDS_B1, sz_B1, _countof(sz_B1));
	LoadStringW(m_hInstResDLL, IDS_B2, sz_B2, _countof(sz_B2));
	LoadStringW(m_hInstResDLL, IDS_B3, sz_B3, _countof(sz_B3));
	LoadStringW(m_hInstResDLL, IDS_C1, sz_C1, _countof(sz_C1));
	LoadStringW(m_hInstResDLL, IDS_C2, sz_C2, _countof(sz_C2));
	LoadStringW(m_hInstResDLL, IDS_C3, sz_C3, _countof(sz_C3));
	LoadStringW(m_hInstResDLL, IDS_D1, sz_D1, _countof(sz_D1));
	LoadStringW(m_hInstResDLL, IDS_D2, sz_D2, _countof(sz_D2));
	LoadStringW(m_hInstResDLL, IDS_D3, sz_D3, _countof(sz_D3));
	LoadStringW(m_hInstResDLL, IDS_D4, sz_D4, _countof(sz_D4));
	LoadStringW(m_hInstResDLL, IDS_D5, sz_D5, _countof(sz_D5));
	LoadStringW(m_hInstResDLL, IDS_D6, sz_D6, _countof(sz_D6));
	LoadStringW(m_hInstResDLL, IDS_D7, sz_D7, _countof(sz_D7));
	LoadStringW(m_hInstResDLL, IDS_D8, sz_D8, _countof(sz_D8));
	LoadStringW(m_hInstResDLL, IDS_D9, sz_D9, _countof(sz_D9));
	LoadStringW(m_hInstResDLL, IDS_D10, sz_D10, _countof(sz_D10));
	LoadStringW(m_hInstResDLL, IDS_D11, sz_D11, _countof(sz_D11));
	LoadStringW(m_hInstResDLL, IDS_D12, sz_D12, _countof(sz_D12));
	LoadStringW(m_hInstResDLL, IDS_D13, sz_D13, _countof(sz_D13));
	LoadStringW(m_hInstResDLL, IDS_D14, sz_D14, _countof(sz_D14));
	LoadStringW(m_hInstResDLL, IDS_D15, sz_D15, _countof(sz_D15));
	LoadStringW(m_hInstResDLL, IDS_D16, sz_D16, _countof(sz_D16));
	LoadStringW(m_hInstResDLL, IDS_D17, sz_D17, _countof(sz_D17));
	LoadStringW(m_hInstResDLL, IDS_D18, sz_D18, _countof(sz_D18));
	LoadStringW(m_hInstResDLL, IDS_D19, sz_D19, _countof(sz_D19));
	LoadStringW(m_hInstResDLL, IDS_D20, sz_D20, _countof(sz_D20));
	LoadStringW(m_hInstResDLL, IDS_D21, sz_D21, _countof(sz_D21));
	LoadStringW(m_hInstResDLL, IDS_D22, sz_D22, _countof(sz_D22));
	LoadStringW(m_hInstResDLL, IDS_D23, sz_D23, _countof(sz_D23));
	LoadStringW(m_hInstResDLL, IDS_D24, sz_D24, _countof(sz_D24));
	LoadStringW(m_hInstResDLL, IDS_D25, sz_D25, _countof(sz_D25));
	LoadStringW(m_hInstResDLL, IDS_D26, sz_D26, _countof(sz_D26));
	LoadStringW(m_hInstResDLL, IDS_D27, sz_D27, _countof(sz_D27));
	LoadStringW(m_hInstResDLL, IDS_D28, sz_D28, _countof(sz_D28));
	LoadStringW(m_hInstResDLL, IDS_E1, sz_E1, _countof(sz_E1));
	LoadStringW(m_hInstResDLL, IDS_E2, sz_E2, _countof(sz_E2));
	LoadStringW(m_hInstResDLL, IDS_F1, sz_F1, _countof(sz_F1));
	LoadStringW(m_hInstResDLL, IDS_F3, sz_F3, _countof(sz_F3));
	LoadStringW(m_hInstResDLL, IDS_F4, sz_F4, _countof(sz_F4));
	LoadStringW(m_hInstResDLL, IDS_F5, sz_F5, _countof(sz_F5));
	LoadStringW(m_hInstResDLL, IDS_F6, sz_F6, _countof(sz_F6));
	LoadStringW(m_hInstResDLL, IDS_F7, sz_F7, _countof(sz_F7));
	LoadStringW(m_hInstResDLL, IDS_F8, sz_F8, _countof(sz_F8));
	LoadStringW(m_hInstResDLL, IDS_F10, sz_F10, _countof(sz_F10));
	LoadStringW(m_hInstResDLL, IDS_F11, sz_F11, _countof(sz_F11));
	LoadStringW(m_hInstResDLL, IDS_G1, sz_G1, _countof(sz_G1));
	LoadStringW(m_hInstResDLL, IDS_G1, sz_G2, _countof(sz_G2));
	LoadStringW(m_hInstResDLL, IDS_G1, sz_G3, _countof(sz_G3));
	LoadStringW(m_hInstResDLL, IDS_H1, sz_H1, _countof(sz_H1));
	LoadStringW(m_hInstResDLL, IDS_H2, sz_H2, _countof(sz_H2));
	LoadStringW(m_hInstResDLL, IDS_H3, sz_H3, _countof(sz_H3));
	LoadStringW(m_hInstResDLL, IDS_H4, sz_H4, _countof(sz_H4));
	LoadStringW(m_hInstResDLL, IDS_H5, sz_H5, _countof(sz_H5));
	LoadStringW(m_hInstResDLL, IDS_H6, sz_H6, _countof(sz_H6));
	LoadStringW(m_hInstResDLL, IDS_H7, sz_H7, _countof(sz_H7));
	LoadStringW(m_hInstResDLL, IDS_H8, sz_H8, _countof(sz_H8));
	LoadStringW(m_hInstResDLL, IDS_H9, sz_H9, _countof(sz_H9));
	LoadStringW(m_hInstResDLL, IDS_H10, sz_H10, _countof(sz_H10));
	LoadStringW(m_hInstResDLL, IDS_H11, sz_H11, _countof(sz_H11));
	LoadStringW(m_hInstResDLL, IDS_H12, sz_H12, _countof(sz_H12));
	LoadStringW(m_hInstResDLL, IDS_H13, sz_H13, _countof(sz_H13));
	LoadStringW(m_hInstResDLL, IDS_H14, sz_H14, _countof(sz_H14));
	LoadStringW(m_hInstResDLL, IDS_H15, sz_H15, _countof(sz_H15));
	LoadStringW(m_hInstResDLL, IDS_H16, sz_H16, _countof(sz_H16));
	LoadStringW(m_hInstResDLL, IDS_H17, sz_H17, _countof(sz_H17));
	LoadStringW(m_hInstResDLL, IDS_H18, sz_H18, _countof(sz_H18));
	LoadStringW(m_hInstResDLL, IDS_H19, sz_H19, _countof(sz_H19));
	LoadStringW(m_hInstResDLL, IDS_H20, sz_H20, _countof(sz_H20));
	LoadStringW(m_hInstResDLL, IDS_H21, sz_H21, _countof(sz_H21));
	LoadStringW(m_hInstResDLL, IDS_H22, sz_H22, _countof(sz_H22));
	LoadStringW(m_hInstResDLL, IDS_H23, sz_H23, _countof(sz_H23));
	LoadStringW(m_hInstResDLL, IDS_H24, sz_H24, _countof(sz_H24));
	LoadStringW(m_hInstResDLL, IDS_H25, sz_H25, _countof(sz_H25));
	LoadStringW(m_hInstResDLL, IDS_H26, sz_H26, _countof(sz_H26));
	LoadStringW(m_hInstResDLL, IDS_H27, sz_H27, _countof(sz_H27));
	LoadStringW(m_hInstResDLL, IDS_H28, sz_H28, _countof(sz_H28));
	LoadStringW(m_hInstResDLL, IDS_H29, sz_H29, _countof(sz_H29));
	LoadStringW(m_hInstResDLL, IDS_H30, sz_H30, _countof(sz_H30));
	LoadStringW(m_hInstResDLL, IDS_H31, sz_H31, _countof(sz_H31));
	LoadStringW(m_hInstResDLL, IDS_H32, sz_H32, _countof(sz_H32));
	LoadStringW(m_hInstResDLL, IDS_H33, sz_H33, _countof(sz_H33));
	LoadStringW(m_hInstResDLL, IDS_H34, sz_H34, _countof(sz_H34));
	LoadStringW(m_hInstResDLL, IDS_H35, sz_H35, _countof(sz_H35));
	LoadStringW(m_hInstResDLL, IDS_H36, sz_H36, _countof(sz_H36));
	LoadStringW(m_hInstResDLL, IDS_H37, sz_H37, _countof(sz_H37));
	LoadStringW(m_hInstResDLL, IDS_H38, sz_H38, _countof(sz_H38));
	LoadStringW(m_hInstResDLL, IDS_H39, sz_H39, _countof(sz_H39));
	LoadStringW(m_hInstResDLL, IDS_H40, sz_H40, _countof(sz_H40));
	LoadStringW(m_hInstResDLL, IDS_H41, sz_H41, _countof(sz_H41));
	LoadStringW(m_hInstResDLL, IDS_H42, sz_H42, _countof(sz_H42));
	LoadStringW(m_hInstResDLL, IDS_H43, sz_H43, _countof(sz_H43));
	LoadStringW(m_hInstResDLL, IDS_H44, sz_H44, _countof(sz_H44));
	LoadStringW(m_hInstResDLL, IDS_H45, sz_H45, _countof(sz_H45));
	LoadStringW(m_hInstResDLL, IDS_H46, sz_H46, _countof(sz_H46));
	LoadStringW(m_hInstResDLL, IDS_H47, sz_H47, _countof(sz_H47));
	LoadStringW(m_hInstResDLL, IDS_H48, sz_H48, _countof(sz_H48));
	LoadStringW(m_hInstResDLL, IDS_H49, sz_H49, _countof(sz_H49));
	LoadStringW(m_hInstResDLL, IDS_H50, sz_H50, _countof(sz_H50));
	LoadStringW(m_hInstResDLL, IDS_H51, sz_H51, _countof(sz_H51));
	LoadStringW(m_hInstResDLL, IDS_H52, sz_H52, _countof(sz_H52));
	LoadStringW(m_hInstResDLL, IDS_H53, sz_H53, _countof(sz_H53));
	LoadStringW(m_hInstResDLL, IDS_H54, sz_H54, _countof(sz_H54));
	LoadStringW(m_hInstResDLL, IDS_H55, sz_H55, _countof(sz_H55));
	LoadStringW(m_hInstResDLL, IDS_H56, sz_H56, _countof(sz_H56));
	LoadStringW(m_hInstResDLL, IDS_H57, sz_H57, _countof(sz_H57));
	LoadStringW(m_hInstResDLL, IDS_H58, sz_H58, _countof(sz_H58));
	LoadStringW(m_hInstResDLL, IDS_H59, sz_H59, _countof(sz_H59));
	LoadStringW(m_hInstResDLL, IDS_H60, sz_H60, _countof(sz_H60));
	LoadStringW(m_hInstResDLL, IDS_H61, sz_H61, _countof(sz_H61));
	LoadStringW(m_hInstResDLL, IDS_H62, sz_H62, _countof(sz_H62));
	LoadStringW(m_hInstResDLL, IDS_H63, sz_H63, _countof(sz_H63));
	LoadStringW(m_hInstResDLL, IDS_H64, sz_H64, _countof(sz_H64));
	LoadStringW(m_hInstResDLL, IDS_H65, sz_H65, _countof(sz_H65));
	LoadStringW(m_hInstResDLL, IDS_H66, sz_H66, _countof(sz_H66));
	LoadStringW(m_hInstResDLL, IDS_H67, sz_H67, _countof(sz_H67));
	LoadStringW(m_hInstResDLL, IDS_H68, sz_H68, _countof(sz_H68));
	LoadStringW(m_hInstResDLL, IDS_H69, sz_H69, _countof(sz_H69));
	LoadStringW(m_hInstResDLL, IDS_H70, sz_H70, _countof(sz_H70));
	LoadStringW(m_hInstResDLL, IDS_H71, sz_H71, _countof(sz_H71));
	LoadStringW(m_hInstResDLL, IDS_H72, sz_H72, _countof(sz_H72));
	LoadStringW(m_hInstResDLL, IDS_H73, sz_H73, _countof(sz_H73));
	LoadStringW(m_hInstResDLL, IDS_I1, sz_I1, _countof(sz_I1));
	LoadStringW(m_hInstResDLL, IDS_I2, sz_I2, _countof(sz_I2));
	LoadStringW(m_hInstResDLL, IDS_I3, sz_I3, _countof(sz_I3));
	LoadStringW(m_hInstResDLL, IDS_I4, sz_I4, _countof(sz_I4));
	LoadStringW(m_hInstResDLL, IDS_J1, sz_J1, _countof(sz_J1));
	LoadStringW(m_hInstResDLL, IDS_J2, sz_J2, _countof(sz_J2));
	LoadStringW(m_hInstResDLL, IDS_K1, sz_K1, _countof(sz_K1));
	LoadStringW(m_hInstResDLL, IDS_K2, sz_K2, _countof(sz_K2));
	LoadStringW(m_hInstResDLL, IDS_K3, sz_K3, _countof(sz_K3));
	LoadStringW(m_hInstResDLL, IDS_L1, sz_L1, _countof(sz_L1));
	LoadStringW(m_hInstResDLL, IDS_L2, sz_L2, _countof(sz_L2));
	LoadStringW(m_hInstResDLL, IDS_L3, sz_L3, _countof(sz_L3));
	LoadStringW(m_hInstResDLL, IDS_L4, sz_L4, _countof(sz_L4));
	LoadStringW(m_hInstResDLL, IDS_L5, sz_L5, _countof(sz_L5));
	LoadStringW(m_hInstResDLL, IDS_L6, sz_L6, _countof(sz_L6));
	LoadStringW(m_hInstResDLL, IDS_L7, sz_L7, _countof(sz_L7));
	LoadStringW(m_hInstResDLL, IDS_L8, sz_L8, _countof(sz_L8));
	LoadStringW(m_hInstResDLL, IDS_L9, sz_L9, _countof(sz_L9));
	LoadStringW(m_hInstResDLL, IDS_L10, sz_L10, _countof(sz_L10));
	LoadStringW(m_hInstResDLL, IDS_L11, sz_L11, _countof(sz_L11));
	LoadStringW(m_hInstResDLL, IDS_L12, sz_L12, _countof(sz_L12));
	LoadStringW(m_hInstResDLL, IDS_L13, sz_L13, _countof(sz_L13));
	LoadStringW(m_hInstResDLL, IDS_L14, sz_L14, _countof(sz_L14));
	LoadStringW(m_hInstResDLL, IDS_L15, sz_L15, _countof(sz_L15));
	LoadStringW(m_hInstResDLL, IDS_L16, sz_L16, _countof(sz_L16));
	LoadStringW(m_hInstResDLL, IDS_L17, sz_L17, _countof(sz_L17));
	LoadStringW(m_hInstResDLL, IDS_L18, sz_L18, _countof(sz_L18));
	LoadStringW(m_hInstResDLL, IDS_L19, sz_L19, _countof(sz_L19));
	LoadStringW(m_hInstResDLL, IDS_L20, sz_L20, _countof(sz_L20));
	LoadStringW(m_hInstResDLL, IDS_L21, sz_L21, _countof(sz_L21));
	LoadStringW(m_hInstResDLL, IDS_L22, sz_L22, _countof(sz_L22));
	LoadStringW(m_hInstResDLL, IDS_L23, sz_L23, _countof(sz_L23));
	LoadStringW(m_hInstResDLL, IDS_L24, sz_L24, _countof(sz_L24));
	LoadStringW(m_hInstResDLL, IDS_L25, sz_L25, _countof(sz_L25));
	LoadStringW(m_hInstResDLL, IDS_L26, sz_L26, _countof(sz_L26));
	LoadStringW(m_hInstResDLL, IDS_L27, sz_L27, _countof(sz_L27));
	LoadStringW(m_hInstResDLL, IDS_L28, sz_L28, _countof(sz_L28));
	LoadStringW(m_hInstResDLL, IDS_L29, sz_L29, _countof(sz_L29));
	LoadStringW(m_hInstResDLL, IDS_L30, sz_L30, _countof(sz_L30));
	LoadStringW(m_hInstResDLL, IDS_L31, sz_L31, _countof(sz_L31));
	LoadStringW(m_hInstResDLL, IDS_L32, sz_L32, _countof(sz_L32));
	LoadStringW(m_hInstResDLL, IDS_L33, sz_L33, _countof(sz_L33));
	LoadStringW(m_hInstResDLL, IDS_L34, sz_L34, _countof(sz_L34));
	LoadStringW(m_hInstResDLL, IDS_L35, sz_L35, _countof(sz_L35));
	LoadStringW(m_hInstResDLL, IDS_L36, sz_L36, _countof(sz_L36));
	LoadStringW(m_hInstResDLL, IDS_L37, sz_L37, _countof(sz_L37));
	LoadStringW(m_hInstResDLL, IDS_L38, sz_L38, _countof(sz_L38));
	LoadStringW(m_hInstResDLL, IDS_L39, sz_L39, _countof(sz_L39));
	LoadStringW(m_hInstResDLL, IDS_L40, sz_L40, _countof(sz_L40));
	LoadStringW(m_hInstResDLL, IDS_L41, sz_L41, _countof(sz_L41));
	LoadStringW(m_hInstResDLL, IDS_L42, sz_L42, _countof(sz_L42));
	LoadStringW(m_hInstResDLL, IDS_L43, sz_L43, _countof(sz_L43));
	LoadStringW(m_hInstResDLL, IDS_L44, sz_L44, _countof(sz_L44));
	LoadStringW(m_hInstResDLL, IDS_L45, sz_L45, _countof(sz_L45));
	LoadStringW(m_hInstResDLL, IDS_L46, sz_L46, _countof(sz_L46));
	LoadStringW(m_hInstResDLL, IDS_L47, sz_L47, _countof(sz_L47));
	LoadStringW(m_hInstResDLL, IDS_L48, sz_L48, _countof(sz_L48));
	LoadStringW(m_hInstResDLL, IDS_L49, sz_L49, _countof(sz_L49));
	LoadStringW(m_hInstResDLL, IDS_L50, sz_L50, _countof(sz_L50));
	LoadStringW(m_hInstResDLL, IDS_L51, sz_L51, _countof(sz_L51));
	LoadStringW(m_hInstResDLL, IDS_L52, sz_L52, _countof(sz_L52));
	LoadStringW(m_hInstResDLL, IDS_L53, sz_L53, _countof(sz_L53));
	LoadStringW(m_hInstResDLL, IDS_L54, sz_L54, _countof(sz_L54));
	LoadStringW(m_hInstResDLL, IDS_L55, sz_L55, _countof(sz_L55));
	LoadStringW(m_hInstResDLL, IDS_L56, sz_L56, _countof(sz_L56));
	LoadStringW(m_hInstResDLL, IDS_L57, sz_L57, _countof(sz_L57));
	LoadStringW(m_hInstResDLL, IDS_L58, sz_L58, _countof(sz_L58));
	LoadStringW(m_hInstResDLL, IDS_L59, sz_L59, _countof(sz_L59));
	LoadStringW(m_hInstResDLL, IDS_L60, sz_L60, _countof(sz_L60));
	LoadStringW(m_hInstResDLL, IDS_L61, sz_L61, _countof(sz_L61));
	LoadStringW(m_hInstResDLL, IDS_L62, sz_L62, _countof(sz_L62));
	LoadStringW(m_hInstResDLL, IDS_L63, sz_L63, _countof(sz_L63));
	LoadStringW(m_hInstResDLL, IDS_L64, sz_L64, _countof(sz_L64));
	LoadStringW(m_hInstResDLL, IDS_L65, sz_L65, _countof(sz_L65));
	LoadStringW(m_hInstResDLL, IDS_L66, sz_L66, _countof(sz_L66));
	LoadStringW(m_hInstResDLL, IDS_L67, sz_L67, _countof(sz_L67));
	LoadStringW(m_hInstResDLL, IDS_L68, sz_L68, _countof(sz_L68));
	LoadStringW(m_hInstResDLL, IDS_L69, sz_L69, _countof(sz_L69));
	LoadStringW(m_hInstResDLL, IDS_L70, sz_L70, _countof(sz_L70));
	LoadStringW(m_hInstResDLL, IDS_L71, sz_L71, _countof(sz_L71));
	LoadStringW(m_hInstResDLL, IDS_L72, sz_L72, _countof(sz_L72));
	LoadStringW(m_hInstResDLL, IDS_L73, sz_L73, _countof(sz_L73));
	LoadStringW(m_hInstResDLL, IDS_L74, sz_L74, _countof(sz_L74));
	LoadStringW(m_hInstResDLL, IDS_L75, sz_L75, _countof(sz_L75));
	LoadStringW(m_hInstResDLL, IDS_L76, sz_L76, _countof(sz_L76));
	LoadStringW(m_hInstResDLL, IDS_L77, sz_L77, _countof(sz_L77));
	LoadStringW(m_hInstResDLL, IDS_L78, sz_L78, _countof(sz_L78));
	LoadStringW(m_hInstResDLL, IDS_L79, sz_L79, _countof(sz_L79));
	LoadStringW(m_hInstResDLL, IDS_L80, sz_L80, _countof(sz_L80));
	LoadStringW(m_hInstResDLL, IDS_L81, sz_L81, _countof(sz_L81));
	LoadStringW(m_hInstResDLL, IDS_L82, sz_L82, _countof(sz_L82));
	LoadStringW(m_hInstResDLL, IDS_L83, sz_L83, _countof(sz_L83));
	LoadStringW(m_hInstResDLL, IDS_L84, sz_L84, _countof(sz_L84));
	LoadStringW(m_hInstResDLL, IDS_L85, sz_L85, _countof(sz_L85));
	LoadStringW(m_hInstResDLL, IDS_L86, sz_L86, _countof(sz_L86));
	LoadStringW(m_hInstResDLL, IDS_L87, sz_L87, _countof(sz_L87));
	LoadStringW(m_hInstResDLL, IDS_L88, sz_L88, _countof(sz_L88));
	LoadStringW(m_hInstResDLL, IDS_L89, sz_L89, _countof(sz_L89));
	LoadStringW(m_hInstResDLL, IDS_L90, sz_L90, _countof(sz_L90));
	LoadStringW(m_hInstResDLL, IDS_L91, sz_L91, _countof(sz_L91));
	LoadStringW(m_hInstResDLL, IDS_L92, sz_L92, _countof(sz_L92));
	LoadStringW(m_hInstResDLL, IDS_L93, sz_L93, _countof(sz_L93));
	LoadStringW(m_hInstResDLL, IDS_L94, sz_L94, _countof(sz_L93));
	LoadStringW(m_hInstResDLL, IDS_L95, sz_ShowOptions, sizeof(sz_ShowOptions) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L96, sz_HideOptions, sizeof(sz_HideOptions) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L97, sz_Computer, sizeof(sz_Computer) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L98, sz_ID, sizeof(sz_ID) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_L99, sz_Port, sizeof(sz_Port) / sizeof(TCHAR));
	LoadStringW(m_hInstResDLL, IDS_M1, sz_M1, _countof(sz_M1));
	LoadStringW(m_hInstResDLL, IDS_M2, sz_M2, _countof(sz_M2));
	LoadStringW(m_hInstResDLL, IDS_M3, sz_M3, _countof(sz_M3));
	LoadStringW(m_hInstResDLL, IDS_M4, sz_M4, _countof(sz_M4));
	LoadStringW(m_hInstResDLL, IDS_M5, sz_M5, _countof(sz_M5));
	LoadStringW(m_hInstResDLL, IDS_N1, sz_N1, _countof(sz_N1));
	LoadStringW(m_hInstResDLL, IDS_N2, sz_N2, _countof(sz_N2));
	LoadStringW(m_hInstResDLL, IDS_N3, sz_N3, _countof(sz_N3));
	LoadStringW(m_hInstResDLL, IDS_N4, sz_N4, _countof(sz_N4));
	LoadStringW(m_hInstResDLL, IDS_N5, sz_N5, _countof(sz_N5));
	LoadStringW(m_hInstResDLL, IDS_N6, sz_N6, _countof(sz_N6));
	LoadStringW(m_hInstResDLL, IDS_N7, sz_N7, _countof(sz_N7));
	LoadStringW(m_hInstResDLL, IDS_N8, sz_N8, _countof(sz_N8));
	LoadStringW(m_hInstResDLL, IDS_N9, sz_N9, _countof(sz_N9));
	LoadStringW(m_hInstResDLL, IDS_N10, sz_N10, _countof(sz_N10));
	LoadStringW(m_hInstResDLL, IDS_N11, sz_N11, _countof(sz_N11));
	LoadStringW(m_hInstResDLL, IDS_N12, sz_N12, _countof(sz_N12));
	LoadStringW(m_hInstResDLL, IDS_N13, sz_N13, _countof(sz_N13));
	LoadStringW(m_hInstResDLL, IDS_N14, sz_N14, _countof(sz_N14));
}

