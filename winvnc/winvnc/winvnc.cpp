/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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


// WinVNC.cpp

// 24/11/97		WEZ

// WinMain and main WndProc for the new version of UltraVNC Server
////////////////////////////
// System headers
#include "stdhdrs.h"
#include "mmsystem.h"
////////////////////////////
// Custom headers
#include "vsocket.h"
#include "winvnc.h"
#include "vncserver.h"
#include "vncmenu.h"
#include "vncinsthandler.h"
#include "vncOSVersion.h"
#include "videodriver.h"
#include <intrin.h>
#include "vncauth.h"
#include "cadthread.h"
#ifdef IPP
void InitIpp();
#endif
#include "VirtualDisplay.h"
#define LOCALIZATION_MESSAGES
#include "Localization.h" // Act : add localization on messages
#include "UltraVNCService.h"
#include "ScSelect.h"
#include "SettingsManager.h"

// Application instance and name
HINSTANCE	hAppInstance;
const char	*szAppName = "WinVNC";
DWORD		mainthreadId;

//adzm 2009-06-20
char* g_szRepeaterHost = NULL;

// sf@2007 - New shutdown order handling stuff (with uvnc_service)
bool			fShutdownOrdered = false;
static HANDLE		hShutdownEvent = NULL;
HANDLE		hShutdownEventcad = NULL;
MMRESULT			mmRes;

void WRITETOLOG(char *szText, int size, DWORD *byteswritten, void *);

//// Handle Old PostAdd message
bool PostAddAutoConnectClient_bool=false;
bool PostAddNewClient_bool=false;
bool PostAddAutoConnectClient_bool_null=false;
bool PostAddConnectClient_bool=false;
bool PostAddConnectClient_bool_null=false;
bool PostAddNewRepeaterClient_bool=false;
bool PostAddNewCloudClient_bool = false;

char pszId_char[20];
#ifdef IPV6V4
VCard32 address_vcard4;
in6_addr address_in6;
#else
VCard32 address_vcard;
#endif
int port_int;


// [v1.0.2-jp1 fix] Load resouce from dll
HINSTANCE	hInstResDLL;
//BOOL G_HTTP;

void Shellexecuteforuiaccess();

void Secure_Plugin_elevated(char *szPlugin);
void Secure_Plugin(char *szPlugin);

//HACK to use name in autoreconnect from service with dyn dns
char dnsname[255];
extern bool PreConnect;
// UltraVNC Server winvnc.exe will also be used for helper exe
// This allow us to minimize the number of seperate exe
#define u16 unsigned short
#define u32 unsigned int
u16 getVolumeHash()
{
	DWORD serialNum = 0;

	// Determine if this volume uses an NTFS file system.
	GetVolumeInformation("c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
	u16 hash = (unsigned short)((serialNum + (serialNum >> 16)) & 0xFFFF);

	return hash;
}

u16 getCpuHash()
{
	int cpuinfo[4] = { 0, 0, 0, 0 };
	__cpuid(cpuinfo, 0);
	u16 hash = 0;
	u16* ptr = (u16*)(&cpuinfo[0]);
	for (u32 i = 0; i < 8; i++)
		hash += ptr[i];

	return hash;
}

bool
Myinit(HINSTANCE hInstance)
{
	setbuf(stderr, 0);

	// [v1.0.2-jp1 fix] Load resouce from dll

	hInstResDLL = NULL;

	 //limit the vnclang.dll searchpath to avoid
	char szCurrentDir[MAX_PATH];
	char szCurrentDir_vnclangdll[MAX_PATH];
	if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
	{
		char* p = strrchr(szCurrentDir, '\\');
		*p = '\0';
	}
	strcpy_s(szCurrentDir_vnclangdll,szCurrentDir);
	strcat_s(szCurrentDir_vnclangdll,"\\");
	strcat_s(szCurrentDir_vnclangdll,"vnclang_server.dll");

	hInstResDLL = LoadLibrary(szCurrentDir_vnclangdll);

	if (hInstResDLL == NULL)
	{
		hInstResDLL = hInstance;
	}
//	RegisterLinkLabel(hInstResDLL);

    //Load all messages from ressource file
    Load_Localization(hInstResDLL) ;
	vnclog.SetFile();
	//vnclog.SetMode(4);
	//vnclog.SetLevel(10);

#ifdef _DEBUG
	{
		// Get current flag
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

		// Turn on leak-checking bit
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

		// Set flag to the new value
		_CrtSetDbgFlag( tmpFlag );
	}
#endif

	// Save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();

	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBoxSecure(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}
	unsigned char key[8] = { 23,82,107,6,35,78,88,7 };
	SYSTEM_INFO siSysInfo;
	GetSystemInfo(&siSysInfo);
	key[0] = LOBYTE((siSysInfo.dwOemId + siSysInfo.dwNumberOfProcessors + siSysInfo.dwProcessorType));
	key[1] = LOBYTE(getVolumeHash());
	key[2] = LOBYTE(getCpuHash());
	key[3] = HIBYTE((siSysInfo.dwOemId + siSysInfo.dwNumberOfProcessors + siSysInfo.dwProcessorType));
	key[4] = HIBYTE(getVolumeHash());
	key[5] = HIBYTE(getCpuHash());
	vncSetDynKey(key);
	return 1;
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

bool return2(bool value)
{
#ifdef CRASHRPT
	crUninstall();
#endif
	if (SettingsManager::getInstance())
		delete SettingsManager::getInstance();
#ifdef SC_20
	if (ScSelect::g_dis_uac)
		ScSelect::Restore_UAC_for_admin_elevated();
#endif // SC_20
	return value;
}

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine2, int iCmdShow)
{
	try {
		if (VNC_OSVersion::getInstance()->OS_XP == true)
			MessageBoxSecure(NULL, "Windows XP requires special build", "Warning", MB_ICONERROR);

		if (VNC_OSVersion::getInstance()->OS_NOTSUPPORTED == true)
		{
			MessageBoxSecure(NULL, "Error OS not supported", "Unsupported OS", MB_ICONERROR);
			return return2(true);
		}
		// make vnc last service to stop
		SetProcessShutdownParameters(0x100, false);
		// handle dpi on aero
		HMODULE hUser32 = LoadLibrary(_T("user32.dll"));
		HMODULE hSHCore = LoadLibrary(_T("SHCore.dll"));

		HRESULT(WINAPI * _SetProcessDpiAwareness)(DWORD value);
		_SetProcessDpiAwareness =
			(HRESULT(WINAPI*)(DWORD))GetProcAddress(hSHCore, "SetProcessDpiAwareness");
		if (_SetProcessDpiAwareness)
			_SetProcessDpiAwareness(2);
		else {
			BOOL(WINAPI * _SetProcessDPIAware)();
			_SetProcessDPIAware = (BOOL(WINAPI*)())GetProcAddress(hUser32, "SetProcessDPIAware");
			if (_SetProcessDPIAware)
				_SetProcessDPIAware();
		}
		if (hUser32)
			FreeLibrary(hUser32);
		if (hSHCore)
			FreeLibrary(hSHCore);

	#ifdef IPP
		InitIpp();
	#endif
	#ifdef CRASHRPT
		CR_INSTALL_INFO info;
		memset(&info, 0, sizeof(CR_INSTALL_INFO));
		info.cb = sizeof(CR_INSTALL_INFO);
		info.pszAppName = _T("UVNC");
		info.pszAppVersion = _T("1.4.4.0-dev");
		info.pszEmailSubject = _T("UltraVNC Server 1.4.4.0-dev Error Report");
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
			return2 1;
		}
	#endif
		bool Injected_autoreconnect = false;
	#ifndef SC_20
		settings->setScExit(false);
		settings->setScPrompt(false);
	#endif // SC_20
		setbuf(stderr, 0);

		// [v1.0.2-jp1 fix] Load resouce from dll
		hInstResDLL = NULL;

		//limit the vnclang.dll searchpath to avoid
		char szCurrentDir[MAX_PATH];
		char szCurrentDir_vnclangdll[MAX_PATH];
		if (GetModuleFileName(NULL, szCurrentDir, MAX_PATH))
		{
			char* p = strrchr(szCurrentDir, '\\');
			*p = '\0';
		}
		strcpy_s(szCurrentDir_vnclangdll, szCurrentDir);
		strcat_s(szCurrentDir_vnclangdll, "\\");
		strcat_s(szCurrentDir_vnclangdll, "vnclang_server.dll");

		hInstResDLL = LoadLibrary(szCurrentDir_vnclangdll);

		if (hInstResDLL == NULL)
		{
			hInstResDLL = hInstance;
		}
		//	RegisterLinkLabel(hInstResDLL);

			//Load all messages from ressource file
		Load_Localization(hInstResDLL);

		char WORKDIR[MAX_PATH];
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
			char* p = strrchr(WORKDIR, '\\');
			if (p == NULL) return return2(0);
			*p = '\0';
		}
		char progname[MAX_PATH];
		strncpy_s(progname, WORKDIR, sizeof progname);
		progname[MAX_PATH - 1] = 0;
		vnclog.SetFile();


	#ifdef _DEBUG
		{
			// Get current flag
			int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

			// Turn on leak-checking bit
			tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

			// Set flag to the new value
			_CrtSetDbgFlag(tmpFlag);
		}
	#endif

		// Save the application instance and main thread id
		hAppInstance = hInstance;
		mainthreadId = GetCurrentThreadId();

		// Initialise the VSocket system
		VSocketSystem socksys;
		if (!socksys.Initialised())
		{
			MessageBoxSecure(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
	#ifdef CRASHRPT
			crUninstall();
	#endif
			return return2(0);
		}
	#ifdef SC_20
		char* szCmdLine = ScSelect::InitSC(hInstance, szCmdLine2);
	#else
		char* szCmdLine = szCmdLine2;
	#endif // SC_20
		// look up the current service name in the registry.
		//serviceHelpers::ExistServiceName(progname, UltraVNCService::service_name);

		// Make the command-line lowercase and parse it
		size_t i;
		for (i = 0; i < strlen(szCmdLine); i++)
		{
			szCmdLine[i] = tolower(szCmdLine[i]);
		}
		BOOL argfound = FALSE;
		for (i = 0; i < strlen(szCmdLine); i++)
		{
			if (szCmdLine[i] <= ' ')
				continue;
			argfound = TRUE;
			if (strncmp(&szCmdLine[i], winvncinipath, strlen(winvncinipath)) == 0)
			{
				char filepath[MAX_PATH];
				i += strlen(winvncinipath);
				char Drv[_MAX_PATH];
				char Path[_MAX_PATH];
				char FileName[_MAX_PATH];
				char FileExt[_MAX_PATH];
				_splitpath_s(&(szCmdLine[i + 1]), Drv, Path, FileName, FileExt);
				char* p = strchr(FileExt, ' ');
				if (p) *p = 0;
				_makepath_s(filepath, Drv, Path, FileName, FileExt);
				g_szIniFile = _strdup(filepath);
				i += strlen(filepath);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				continue;
			}

			if (strncmp(&szCmdLine[i], winvncSettingshelper, strlen(winvncSettingshelper)) == 0)
			{
				Sleep(3000);
				char mycommand[MAX_PATH];
				i += strlen(winvncSettingshelper);
				strcpy_s(mycommand, &(szCmdLine[i + 1]));
				settingsHelpers::Set_settings_as_admin(mycommand);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncStopserviceHelper, strlen(winvncStopserviceHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::Set_stop_service_as_admin();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
	#endif // SC_20

			if (strncmp(&szCmdLine[i], winvncKill, strlen(winvncKill)) == 0)
			{
				static HANDLE		hShutdownEventTmp;
				hShutdownEventTmp = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");
				SetEvent(hShutdownEventTmp);
				CloseHandle(hShutdownEventTmp);

				//adzm 2010-02-10 - Finds the appropriate VNC window for any process. Sends this message to all of them!
				// do removed, loops forever with cpu 100
				HWND hservwnd = NULL;
				hservwnd = postHelper::FindWinVNCWindow(false);
				if (hservwnd != NULL)
				{
					PostMessage(hservwnd, WM_COMMAND, 40002, 0);
					PostMessage(hservwnd, WM_CLOSE, 0, 0);
				}
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenhomepage, strlen(winvncopenhomepage)) == 0)
			{
				Open_homepage();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenforum, strlen(winvncopenforum)) == 0)
			{
				Open_forum();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopengithub, strlen(winvncopengithub)) == 0)
			{
				Open_github();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenmastodon, strlen(winvncopenmastodon)) == 0)
			{
				Open_mastodon();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenfacebook, strlen(winvncopenfacebook)) == 0)
			{
				Open_facebook();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenxtwitter, strlen(winvncopenxtwitter)) == 0)
			{
				Open_xtwitter();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenreddit, strlen(winvncopenreddit)) == 0)
			{
				Open_reddit();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenopenhub, strlen(winvncopenopenhub)) == 0)
			{
				Open_openhub();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

	#ifdef SC_20
			if (strncmp(&szCmdLine[i], "-noregistry", strlen("-noregistry")) == 0)
			{
				i += strlen("-noregistry");
				continue;
			}
			if (strncmp(&szCmdLine[i], "-secureplugin", strlen("-secureplugin")) == 0)
			{
				i += strlen("-secureplugin");
				settings->setUseDSMPlugin(true);
				continue;
			}
	#endif // SC_20
	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncStartserviceHelper, strlen(winvncStartserviceHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::Set_start_service_as_admin();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncInstallServiceHelper, strlen(winvncInstallServiceHelper)) == 0)
			{
				//Sleeps are realy needed, else runas fails...
				Sleep(3000);
				serviceHelpers::Set_install_service_as_admin();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncUnInstallServiceHelper, strlen(winvncUnInstallServiceHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::Set_uninstall_service_as_admin();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncSoftwarecadHelper, strlen(winvncSoftwarecadHelper)) == 0)
			{
				Sleep(3000);
				vncCad::Enable_softwareCAD_elevated();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncdelSoftwarecadHelper, strlen(winvncdelSoftwarecadHelper)) == 0)
			{
				Sleep(3000);
				vncCad::delete_softwareCAD_elevated();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncRebootSafeHelper, strlen(winvncRebootSafeHelper)) == 0)
			{
				Sleep(3000);
				UltraVNCService::Reboot_in_safemode_elevated();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncRebootForceHelper, strlen(winvncRebootForceHelper)) == 0)
			{
				Sleep(3000);
				UltraVNCService::Reboot_with_force_reboot_elevated();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncSecurityEditorHelper, strlen(winvncSecurityEditorHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::winvncSecurityEditorHelper_as_admin();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
	#endif // SC_20
			if (strncmp(&szCmdLine[i], winvncSecurityEditor, strlen(winvncSecurityEditor)) == 0)
			{
				typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
				vncEditSecurityFn vncEditSecurity = 0;
				char szCurrentDirl[MAX_PATH];
				if (GetModuleFileName(NULL, szCurrentDirl, MAX_PATH)) {
					char* p = strrchr(szCurrentDirl, '\\');
					*p = '\0';
					strcat_s(szCurrentDirl, "\\authSSP.dll");
				}
				HMODULE hModule = LoadLibrary(szCurrentDirl);
				if (hModule) {
					vncEditSecurity = (vncEditSecurityFn)GetProcAddress(hModule, "vncEditSecurity");
					CoInitialize(NULL);
					vncEditSecurity(NULL, hAppInstance);
					CoUninitialize();
					FreeLibrary(hModule);
				}
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncSettings, strlen(winvncSettings)) == 0)
			{
				char mycommand[MAX_PATH];
				i += strlen(winvncSettings);
				strcpy_s(mycommand, &(szCmdLine[i + 1]));
				settingsHelpers::Real_settings(mycommand);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], dsmpluginhelper, strlen(dsmpluginhelper)) == 0)
			{
				char mycommand[MAX_PATH];
				i += strlen(dsmpluginhelper);
				strcpy_s(mycommand, &(szCmdLine[i + 1]));
				Secure_Plugin_elevated(mycommand);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], dsmplugininstance, strlen(dsmplugininstance)) == 0)
			{
				char mycommand[MAX_PATH];
				i += strlen(dsmplugininstance);
				strcpy_s(mycommand, &(szCmdLine[i + 1]));
				Secure_Plugin(mycommand);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncSoftwarecad, strlen(winvncSoftwarecad)) == 0)
			{
				vncCad::Enable_softwareCAD();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncdelSoftwarecad, strlen(winvncdelSoftwarecad)) == 0)
			{
				vncCad::delete_softwareCAD();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncRebootSafe, strlen(winvncRebootSafe)) == 0)
			{
				UltraVNCService::Reboot_in_safemode();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncRebootForce, strlen(winvncRebootForce)) == 0)
			{
				UltraVNCService::Reboot_with_force_reboot();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncStopservice, strlen(winvncStopservice)) == 0)
			{
				serviceHelpers::Real_stop_service();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncStartservice, strlen(winvncStartservice)) == 0)
			{
				serviceHelpers::Real_start_service();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncInstallDriver, strlen(winvncInstallDriver)) == 0) {
				VirtualDisplay::InstallDriver(true);
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncInstallService, strlen(winvncInstallService)) == 0)
			{
				// rest of command line service name, if provided.
				char* pServiceName = &szCmdLine[i];
				// skip over command switch, find next whitepace
				while (*pServiceName && !isspace(*(unsigned char*)pServiceName))
					++pServiceName;

				// skip past whitespace to service name
				while (*pServiceName && isspace(*(unsigned char*)pServiceName))
					++pServiceName;

				// strip off any quotes
				if (*pServiceName && *pServiceName == '\"')
					++pServiceName;

				if (*pServiceName)
				{
					// look for trailing quote, if found, terminate the string there.
					char* pQuote = pServiceName;
					pQuote = strrchr(pServiceName, '\"');
					if (pQuote)
						*pQuote = 0;
				}
				// if a service name is supplied, and it differs except in case from
				// the default, use the supplied service name instead
				if (*pServiceName && (_strcmpi(pServiceName, UltraVNCService::service_name) != 0))
				{
					strncpy_s(UltraVNCService::service_name, 256, pServiceName, 256);
					UltraVNCService::service_name[255] = 0;
				}
				UltraVNCService::install_service();
				Sleep(2000);
				char command[MAX_PATH + 32]; // 29 January 2008 jdp
				_snprintf_s(command, sizeof command, "net start \"%s\"", UltraVNCService::service_name);
				WinExec(command, SW_HIDE);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncUnInstallService, strlen(winvncUnInstallService)) == 0)
			{
				char command[MAX_PATH + 32]; // 29 January 2008 jdp
				// rest of command line service name, if provided.
				char* pServiceName = &szCmdLine[i];
				// skip over command switch, find next whitepace
				while (*pServiceName && !isspace(*(unsigned char*)pServiceName))
					++pServiceName;

				// skip past whitespace to service name
				while (*pServiceName && isspace(*(unsigned char*)pServiceName))
					++pServiceName;

				// strip off any quotes
				if (*pServiceName && *pServiceName == '\"')
					++pServiceName;

				if (*pServiceName)
				{
					// look for trailing quote, if found, terminate the string there.
					char* pQuote = pServiceName;
					pQuote = strrchr(pServiceName, '\"');
					if (pQuote)
						*pQuote = 0;
				}

				if (*pServiceName && (_strcmpi(pServiceName, UltraVNCService::service_name) != 0))
				{
					strncpy_s(UltraVNCService::service_name, 256, pServiceName, 256);
					UltraVNCService::service_name[255] = 0;
				}
				_snprintf_s(command, sizeof command, "net stop \"%s\"", UltraVNCService::service_name);
				WinExec(command, SW_HIDE);
				UltraVNCService::uninstall_service();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
	#endif // SC_20
			if (strncmp(&szCmdLine[i], winvncPreConnect, strlen(winvncPreConnect)) == 0)
			{
				i += strlen(winvncPreConnect);
				PreConnect = true;
				continue;
			}
			if (strncmp(&szCmdLine[i], winvncRunService, strlen(winvncRunService)) == 0)
			{
				//Run as service
				if (!Myinit(hInstance)) return return2(0);
				settings->RunningFromExternalService(true);
				int return2value = WinVNCAppMain();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(return2value);
			}

			if (strncmp(&szCmdLine[i], winvncRunServiceRdp, strlen(winvncRunServiceRdp)) == 0)
			{
				//Run as service
				if (!Myinit(hInstance)) return return2(0);
				settings->RunningFromExternalService(true);
				settings->RunningFromExternalServiceRdp(true);
				int return2value = WinVNCAppMain();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(return2value);
			}
	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncStartService, strlen(winvncStartService)) == 0)
			{
				UltraVNCService::start_service(szCmdLine);
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
	#endif // SC_20
			if (strncmp(&szCmdLine[i], winvncRunAsUserApp, strlen(winvncRunAsUserApp)) == 0)
			{
				// WinVNC is being run as a user-level program
				if (!Myinit(hInstance)) return return2(0);
				int return2value = WinVNCAppMain();
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(return2value);
			}

			if (strncmp(&szCmdLine[i], winvncSCexit, strlen(winvncSCexit)) == 0)
			{
				settings->setScExit(true);
				i += strlen(winvncSCexit);
				continue;
			}

			if (strncmp(&szCmdLine[i], winvncSCprompt, strlen(winvncSCprompt)) == 0)
			{
				settings->setScPrompt(true);
				i += strlen(winvncSCprompt);
				continue;
			}

			if (strncmp(&szCmdLine[i], winvncmulti, strlen(winvncmulti)) == 0)
			{
				allowMultipleInstances = true;
				i += strlen(winvncmulti);
				continue;
			}
			/*
			if (strncmp(&szCmdLine[i], winvnchttp, strlen(winvnchttp)) == 0)
			{
				G_HTTP=true;
				i+=strlen(winvnchttp);
				continue;
			}*/

			if (strncmp(&szCmdLine[i], winvncStopReconnect, strlen(winvncStopReconnect)) == 0)
			{
				i += strlen(winvncStopReconnect);
				postHelper::PostAddStopConnectClientAll();
				continue;
			}

			if (strncmp(&szCmdLine[i], winvncAutoReconnect, strlen(winvncAutoReconnect)) == 0)
			{
				// Note that this "autoreconnect" param MUST be BEFORE the "connect" one
				// on the command line !
				// wa@2005 -- added support for the AutoReconnectId
				i += strlen(winvncAutoReconnect);
				Injected_autoreconnect = true;
				size_t start, end;
				char* pszId = NULL;
				start = i;
				// skip any spaces and grab the parameter
				while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;

				if (strncmp(&szCmdLine[start], winvncAutoReconnectId, strlen(winvncAutoReconnectId)) == 0)
				{
					end = start;
					while (szCmdLine[end] > ' ') end++;

					if (end - start > 0)
					{

						pszId = new char[end - start + 1];

						strncpy_s(pszId, end - start + 1, &(szCmdLine[start]), end - start);
						pszId[end - start] = 0;
						pszId = _strupr(pszId);
					}
					//multiple spaces between autoreconnect and id
					i = end;
				}// end of condition we found the ID: parameter

				// NOTE:  id must be NULL or the ID:???? (pointer will get deleted when message is processed)
				// We can not contact a runnning service, permissions, so we must store the settings
				// and process until the vncmenu has been started

				if (!postHelper::PostAddAutoConnectClient(pszId))
				{
					PostAddAutoConnectClient_bool = true;
					if (pszId == NULL)
					{
						PostAddAutoConnectClient_bool_null = true;
						PostAddAutoConnectClient_bool = false;
					}
					else
					{
						strcpy_s(pszId_char, pszId);
						//memory leak fix
						delete[] pszId; pszId = NULL;
					}
				}
				if (pszId != NULL) delete[] pszId; pszId = NULL;
				continue;
			}

			if (strncmp(&szCmdLine[i], winvncReconnectId, strlen(winvncReconnectId)) == 0)
			{
				i += strlen("-");
				size_t start, end;
				char* pszId = NULL;
				start = i;
				end = start;
				while (szCmdLine[end] > ' ') end++;
				if (end - start > 0)
				{
					pszId = new char[end - start + 1];
					if (pszId != 0)
					{
						strncpy_s(pszId, end - start + 1, &(szCmdLine[start]), end - start);
						pszId[end - start] = 0;
						pszId = _strupr(pszId);
					}
				}
				i = end;
				if (!postHelper::PostAddConnectClient(pszId))
				{
					PostAddConnectClient_bool = true;
					if (pszId == NULL)
					{
						PostAddConnectClient_bool_null = true;
						PostAddConnectClient_bool = false;
					}
					else
					{
						strcpy_s(pszId_char, pszId);
						//memory leak fix
						delete[] pszId; pszId = NULL;
					}
				}
				if (pszId != NULL) delete[] pszId; pszId = NULL;
				continue;
			}

			if (strncmp(&szCmdLine[i], winvncConnect, strlen(winvncConnect)) == 0)
			{
				if (!Injected_autoreconnect)
				{
					postHelper::PostAddStopConnectClient();
				}
				// Add a new client to an existing copy of winvnc
				i += strlen(winvncConnect);

				// First, we have to parse the command line to get the filename to use
				size_t start, end;
				start = i;
				while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
				end = start;
				while (szCmdLine[end] > ' ') end++;

				// Was there a hostname (and optionally a port number) given?
				if (end - start > 0)
				{
					char* name = new char[end - start + 1];
					char* name2 = new char[end - start + 1];
					if (name != 0) {
						strncpy_s(name, end - start + 1, &(szCmdLine[start]), end - start);
						name[end - start] = 0;
						strcpy_s(name2, end - start + 1, name);
						//detect braceletes in IPv6 address or remove port number from name
						char* bs = strchr(name, '[');
						char* be = strchr(name, ']');
						if (bs && be) {
							strncpy_s(name2, end - start + 1, be + 1, strlen(be));
							*be = '\0';
							strcpy_s(name, end - start + 1, bs + 1);
						}
						else {
							char* portp = strchr(name, ':');
							if (portp) {
								*portp++ = '\0';
							}
						}
						int port = INCOMING_PORT_OFFSET;
						char* portp = strchr(name2, ':');
						if (portp) {
							*portp++ = '\0';
							if (*portp == ':') {
								port = atoi(++portp);	// Port number after "::"
							}
							else {
								port = atoi(portp);	// Display number after ":"
							}
						}
						delete[] name2;
						vnclog.Print(LL_STATE, VNCLOG("test... %s %d\n"), name, port);
						strcpy_s(dnsname, name);
	#ifdef IPV6V4
						if (settings->getIPV6())
						{
							in6_addr address;
							memset(&address, 0, sizeof(address));
							if (VSocket::Resolve6(name, &address))
							{
								vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient III \n"));
								if (!postHelper::PostAddNewClientInit6(&address, port))
								{
									PostAddNewClient_bool = true;
									port_int = port;
									address_in6 = address;
								}
							}
							else
							{
								//ask for host,port
								PostAddNewClient_bool = true;
								port_int = 0;
								memset(&address_in6, 0, sizeof(address_in6));
								Sleep(2000);
								delete[] name;
								return return2(0);
							}
						}
						if (port_int == 0)
						{
							VCard32 address = VSocket::Resolve4(name);
							if (address != 0) {
								// Post the IP address to the server
								// We can not contact a runnning service, permissions, so we must store the settings
								// and process until the vncmenu has been started
								vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient III \n"));
								if (!postHelper::PostAddNewClientInit4(address, port))
								{
									PostAddNewClient_bool = true;
									port_int = port;
									address_vcard4 = address;
								}
							}
							else
							{
								//ask for host,port
								PostAddNewClient_bool = true;
								port_int = 0;
								address_vcard4 = 0;
								Sleep(2000);
								delete[] name;
								return return2(0);
							}
						}

						delete[] name;
	#else
						VCard32 address = VSocket::Resolve(name);
	#ifdef SC_20
						if (address == 0) {
							char text[1024]{};
							sprintf(text, " Hostnamee (%s) could not be resolved", name);
							MessageBox(NULL, text, szAppName, MB_ICONEXCLAMATION | MB_OK);
							delete[] name;
							return return2(0);
						}
	#endif // SC_20
						delete[] name;
						if (address != 0) {
							// Post the IP address to the server
							// We can not contact a runnning service, permissions, so we must store the settings
							// and process until the vncmenu has been started
							vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient III \n"));
							if (!postHelper::PostAddNewClientInit(address, port))
							{
								PostAddNewClient_bool = true;
								port_int = port;
								address_vcard = address;
							}
						}
						else
						{
							//ask for host,port
							PostAddNewClient_bool = true;
							port_int = 0;
							address_vcard = 0;
							Sleep(2000);
							//Beep(200,1000);
							return return2(0);
						}
	#endif
					}
					i = end;
					continue;
				}
				else
				{
					// Tell the server to show the Add New Client dialog
					// We can not contact a runnning service, permissions, so we must store the settings
					// and process until the vncmenu has been started
					vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient IIII\n"));
	#ifdef IPV6V4
					if (!postHelper::PostAddNewClient4(0, 0))
					{
						PostAddNewClient_bool = true;
						port_int = 0;
						if (settings->getIPV6())
							memset(&address_in6, 0, sizeof(address_in6));
						else
							address_vcard4 = 0;
					}
	#else
					if (!postHelper::PostAddNewClient(0, 0))
					{
						PostAddNewClient_bool = true;
						port_int = 0;
						address_vcard = 0;
					}
	#endif
				}
				continue;
			}


			//adzm 2009-06-20
			if (strncmp(&szCmdLine[i], winvncRepeater, strlen(winvncRepeater)) == 0)
			{
				// set the default repeater host
				i += strlen(winvncRepeater);

				// First, we have to parse the command line to get the host to use
				size_t start, end;
				start = i;
				while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
				end = start;
				while (szCmdLine[end] > ' ') end++;

				// Was there a hostname (and optionally a port number) given?
				if (end - start > 0)
				{
					if (g_szRepeaterHost) {
						delete[] g_szRepeaterHost;
						g_szRepeaterHost = NULL;
					}
					g_szRepeaterHost = new char[end - start + 1];
					if (g_szRepeaterHost != 0) {
						strncpy_s(g_szRepeaterHost, end - start + 1, &(szCmdLine[start]), end - start);
						g_szRepeaterHost[end - start] = 0;

						// We can not contact a runnning service, permissions, so we must store the settings
						// and process until the vncmenu has been started
						vnclog.Print(LL_INTERR, VNCLOG("PostAddNewRepeaterClient I\n"));
						if (!postHelper::PostAddNewRepeaterClient())
						{
							PostAddNewRepeaterClient_bool = true;
							port_int = 0;
	#ifdef IPV6V4
							address_vcard4 = 0;
							memset(&address_in6, 0, sizeof(address_in6));
	#else
							address_vcard = 0;
	#endif
						}
					}
					i = end;
					continue;
				}
				continue;
			}

			// Either the user gave the -help option or there is something odd on the cmd-line!

			// Show the usage dialog
			MessageBoxSecure(NULL, winvncUsageText, sz_ID_WINVNC_USAGE, MB_OK | MB_ICONINFORMATION);
			break;
		};

		// If no arguments were given then just run
		if (!argfound)
		{
			if (!Myinit(hInstance))
			{
	#ifdef CRASHRPT
				crUninstall();
	#endif
				return return2(0);
			}
			int return2value = WinVNCAppMain();
	#ifdef CRASHRPT
			crUninstall();
	#endif
			return return2(return2value);
		}
	}
	catch (...) {
		return return2(0);
	}

	return return2(0);
}

// rdv&sf@2007 - New TrayIcon impuDEsktop/impersonation thread stuff

DWORD WINAPI imp_desktop_thread(LPVOID lpParam)
{
	vncServer *server = (vncServer *)lpParam;
	HDESK desktop;
	//vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
	//vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);

	if (desktop == NULL)
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop Error \n"));
	else
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop OK\n"));

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;

	char new_name[256];

	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
	}

	vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
	}

//	ImpersonateCurrentUser_();

	char m_username[UNLEN+1];
	HWINSTA station = GetProcessWindowStation();
	if (station != NULL)
	{
		DWORD usersize;
		GetUserObjectInformation(station, UOI_USER_SID, NULL, 0, &usersize);
		SetLastError(0);
		if (usersize != 0)
		{
			DWORD length = sizeof(m_username);
			if (GetUserName(m_username, &length) == 0)
			{
				UINT error = GetLastError();
				if (error != ERROR_NOT_LOGGED_ON)
				{
					vnclog.Print(LL_INTERR, VNCLOG("getusername error %d\n"), GetLastError());
					SetThreadDesktop(old_desktop);
                	CloseDesktop(desktop);
					Sleep(500);
					return FALSE;
				}
			}
		}
	}
    vnclog.Print(LL_INTERR, VNCLOG("Username %s \n"),m_username);

	// Create Tray icon and menu
	auto menu = std::make_unique<vncMenu>(server);
	if(menu == NULL){
		vnclog.Print(LL_INTERR, VNCLOG("failed to create tray menu\n"));
		PostQuitMessage(0);
	}

	// This is a good spot to handle the old PostAdd messages
	if (PostAddAutoConnectClient_bool)
		postHelper::PostAddAutoConnectClient( pszId_char );
	if (PostAddAutoConnectClient_bool_null)
		postHelper::PostAddAutoConnectClient( NULL );

	if (PostAddConnectClient_bool)
		postHelper::PostAddConnectClient( pszId_char );
	if (PostAddConnectClient_bool_null)
		postHelper::PostAddConnectClient( NULL );

	if (PostAddNewClient_bool)
	{
		PostAddNewClient_bool=false;
		vnclog.Print(LL_INTERR, VNCLOG("PostAddNewClient IIIII\n"));
#ifdef IPV6V4
		if (settings->getIPV6())
		{
			postHelper::PostAddNewClient6(&address_in6, port_int);
		}
		else
		{
			postHelper::PostAddNewClient4(address_vcard4, port_int);
		}
#else
		postHelper::PostAddNewClient(address_vcard, port_int);
#endif
	}
	//adzm 2009-06-20
	if (PostAddNewRepeaterClient_bool)
	{
		PostAddNewRepeaterClient_bool=false;
		vnclog.Print(LL_INTERR, VNCLOG("PostAddNewRepeaterClient II\n"));
		postHelper::PostAddNewRepeaterClient();
	}
	if (PostAddNewCloudClient_bool)
	{
		PostAddNewCloudClient_bool = false;
		vnclog.Print(LL_INTERR, VNCLOG("PostAddNewCloudClient II\n"));
		postHelper::PostAddNewCloudClient();
	}
	bool Runonce=false;
	MSG msg;
	while (GetMessage(&msg,0,0,0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (fShutdownOrdered && !Runonce)
		{
			Runonce=true;
			if (menu) menu->Shutdown(true);
		}

		if (hShutdownEvent)
		{
			// vnclog.Print(LL_INTERR, VNCLOG("****************** SDTimer tic\n"));
			DWORD result = WaitForSingleObject(hShutdownEvent, 1);
			if (WAIT_OBJECT_0 == result)
			{
				ResetEvent(hShutdownEvent);
				fShutdownOrdered = true;
				//vnclog.Print(LL_INTERR, VNCLOG("****************** WaitForSingleObject - Shutdown server\n"));
			}
		}
	}

	//vnclog.Print(LL_INTERR, VNCLOG("GetMessage stop \n"));
	SetThreadDesktop(old_desktop);
	CloseDesktop(desktop);
//	RevertToSelf();
	return 0;
}

// This is the main routine for UltraVNC Server when running as an application
// (under Windows 95 or Windows NT)
// Under NT, UltraVNC Server can also run as a service. The WinVNCServerMain routine,
// defined in the vncService header, is used instead when running as a service.

int WinVNCAppMain()
{
	vnclog.SetMode(settings->getDebugMode());
	vnclog.SetPath(settings->getDebugPath());
	vnclog.SetLevel(settings->getDebugLevel());
	vnclog.SetVideo(settings->getAvilog());

	vnclog.Print(-1, VNCLOG("WinVNCAPPMain-----Application started\n"));
#ifdef CRASH_ENABLED
	LPVOID lpvState = Install(NULL,  "rudi.de.vos@skynet.be", "UltraVNC");
#endif

	// Set this process to be the last application to be shut down.
	// Check for previous instances of UltraVNC Server!
	auto  instancehan = std::make_unique<vncInstHandler>();
	if (!allowMultipleInstances) // this allow to overwrite the multiple instance check
	{
		if (!instancehan->Init())
		{
    		vnclog.Print(LL_INTINFO, VNCLOG("%s -- exiting\n"), sz_ID_ANOTHER_INST);
			// We don't allow multiple instances!
			if (!settings->RunningFromExternalService())
				MessageBoxSecure(NULL, sz_ID_ANOTHER_INST, szAppName, MB_OK);
			return 0;
		}
	}

	//vnclog.Print(LL_INTINFO, VNCLOG("***** DBG - Previous instance checked - Trying to create server\n"));
	// CREATE SERVER
	vncServer server;

	// Set the name and port number
	server.SetName(szAppName);
	vnclog.Print(LL_STATE, VNCLOG("server created ok\n"));
	///uninstall driver before cont

	// sf@2007 - New impersonation thread stuff for Tray icon & menu
	// Subscribe to shutdown event
	hShutdownEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "Global\\SessionEventUltra");
	if (hShutdownEvent) ResetEvent(hShutdownEvent);
	//vnclog.Print(LL_STATE, VNCLOG("***************** SDEvent created \n"));
	// Create the timer that looks periodicaly for shutdown event
	mmRes = -1;
	//InitSDTimer();

	while ( !fShutdownOrdered)
	{
		//vnclog.Print(LL_STATE, VNCLOG("################## Creating Imp Thread : %d \n"), nn);

		HANDLE threadHandle;
		DWORD dwTId;
		threadHandle = CreateThread(NULL, 0, imp_desktop_thread, &server, 0, &dwTId);

		if (threadHandle)
		{
			WaitForSingleObject( threadHandle, INFINITE );
			CloseHandle(threadHandle);
		}
		vnclog.Print(LL_STATE, VNCLOG("################## Closing Imp Thread\n"));
	}
	fShutdownOrdered = true;

	if (hShutdownEvent)CloseHandle(hShutdownEvent);
	vnclog.Print(LL_STATE, VNCLOG("################## SHUTING DOWN SERVER ####################\n"));

	//adzm 2009-06-20
	if (g_szRepeaterHost) {
		delete[] g_szRepeaterHost;
		g_szRepeaterHost = NULL;
	}

	if (VNC_OSVersion::getInstance())
		delete VNC_OSVersion::getInstance();
	return 1;
};
