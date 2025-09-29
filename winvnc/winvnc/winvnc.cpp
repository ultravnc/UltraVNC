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
#include "VirtualDisplay.h"
#define LOCALIZATION_MESSAGES
#include "Localization.h" // Act : add localization on messages
#include "UltraVNCService.h"
#include "ScSelect.h"
#include "SettingsManager.h"
#include <commctrl.h>
#include "shlwapi.h"
#include <shlobj.h>
#include <fstream>
#include <direct.h>
#include "credentials.h"

#pragma comment (lib, "comctl32")

// Application instance and name
HINSTANCE	hAppInstance;
const char	*szAppName = "WinVNC";
DWORD		mainthreadId;
char configFile[256] = { 0 };
int configfileskip = 0;
bool showSettings = false;
char winvncFolder[MAX_PATH];

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
VCard32 address_vcard4;
in6_addr address_in6;
VCard32 address_vcard;
int port_int;


// [v1.0.2-jp1 fix] Load resouce from dll
HINSTANCE	hInstResDLL;
//BOOL G_HTTP;

void Shellexecuteforuiaccess();

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
	char szCurrentDir_vnclangdll[MAX_PATH];
	char szCurrentDir[MAX_PATH];
	strcpy_s(szCurrentDir, winvncFolder);
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

bool return2(bool value)
{
	if (SettingsManager::getInstance())
		delete SettingsManager::getInstance();
#ifdef SC_20
	if (ScSelect::g_dis_uac)
		ScSelect::Restore_UAC_for_admin_elevated();
#endif // SC_20
	return value;
}

void replaceFilename(char* path, const char* newFilename) {
	char* lastSlash = strrchr(path, '\\'); // Find the last '/'
	if (lastSlash) {
		*(lastSlash + 1) = '\0'; // Truncate after the last '/'
		strcat(path, newFilename); // Append the new filename
	}
	else {
		// No '/' found, replace the whole string
		strcpy(path, newFilename);
	}
}

void extractConfig(char* szCmdLine)
{
	size_t i = 0;
	while (szCmdLine[i] != '\0') {
		if (strncmp(&szCmdLine[i], winvncConfig, strlen(winvncConfig)) == 0) {
			i += strlen(winvncConfig); // Skip the -config part
			// Skip any leading spaces
			while (szCmdLine[i] == ' ') {
				i++;
				configfileskip++;
			}

			size_t pathLength = 0; // Variable to store the path length

			// Check if the value is quoted
			if (szCmdLine[i] == '"') {
				i++; // Skip the opening quote
				configfileskip++;
				const char* start = &szCmdLine[i];
				const char* end = strchr(start, '"'); // Find the closing quote
				if (end) {
					pathLength = end - start; // Calculate the length of the path
					strncpy(configFile, start, pathLength); // Copy the path into the char array
					configFile[pathLength] = '\0'; // Null-terminate the path
					i += pathLength + 1; // Move i past the closing quote
					configfileskip += pathLength + 1;
				}
			}
			else {
				// Unquoted value
				const char* start = &szCmdLine[i];
				const char* end = strchr(start, ' '); // Find the next space
				if (end) {
					pathLength = end - start; // Calculate the length of the path
					strncpy(configFile, start, pathLength); // Copy the path into the char array
					configFile[pathLength] = '\0'; // Null-terminate the path
					i += pathLength; // Move i past the path
					configfileskip += pathLength;
				}
				else {
					// Path is the rest of the string
					strcpy(configFile, &szCmdLine[i]); // Copy the rest of the string into the path
					pathLength = strlen(&szCmdLine[i]);
					i += pathLength; // Move i past the path
					configfileskip += pathLength;
				}
			}
			break; // Exit the loop after processing -config
		}
		i++;
	}
	if (strlen(configFile) == 0) {
		char appdataPath[MAX_PATH]{};
		char appdataFolder[MAX_PATH]{};
		char programdataPath[MAX_PATH]{};
		char szCurrentDir[MAX_PATH]{};
		strcpy_s(szCurrentDir, winvncFolder);		
		strcat_s(szCurrentDir, "\\");
		strcat_s(szCurrentDir, INIFILE_NAME);
#ifndef SC_20
		SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programdataPath);
		SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdataPath);
		SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdataFolder);
		strcat_s(programdataPath, "\\UltraVNC");
		strcat_s(programdataPath, "\\");
		strcat_s(programdataPath, INIFILE_NAME);
		strcat_s(appdataPath, "\\UltraVNC");
		strcat_s(appdataPath, "\\");
		strcat_s(appdataPath, INIFILE_NAME);


		std::ifstream file;
		file.open(appdataPath);
		if (file.good()) {
			strcpy_s(configFile, appdataPath);
			showSettings = true;
			vnclog.Print(LL_LOGSCREEN, "using config file %s", configFile);
		}
		else {
			file.clear();
			vnclog.Print(LL_LOGSCREEN, "config file not found %s", appdataPath);
			file.open(programdataPath);
			if (file.good()) {
				strcpy_s(configFile, programdataPath);
				//only admins can edit programdata
				showSettings = Credentials::RunningAsAdministrator(false);
				vnclog.Print(LL_LOGSCREEN, "using config file %s", configFile);
			}
			else {
				file.clear();
				vnclog.Print(LL_LOGSCREEN, "config file not found %s", programdataPath);
				file.open(szCurrentDir);
				if (file.good()) {
					strcpy_s(configFile, szCurrentDir);
					showSettings = true;
					vnclog.Print(LL_LOGSCREEN, "using config file %s", configFile);
				}
				else {
					//nothing found, default to appdata
					vnclog.Print(LL_LOGSCREEN, "config file not found %s", szCurrentDir);
					strcpy_s(configFile, appdataPath);
					_mkdir(appdataFolder);
					vnclog.Print(LL_LOGSCREEN, "creating config file %s", configFile);
					showSettings = true;
				}
			}
		}
#else
		strcpy_s(configFile, szCurrentDir);
#endif
	}
	else {
		showSettings = true; // service
	}
	char logFile[MAX_PATH];
	strcpy(logFile, configFile);
	replaceFilename(logFile, "mslogon.log");
	settings->setLogFile(logFile);
	settings->setShowSettings(showSettings);
}
	

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine2, int iCmdShow)
{
	if (GetModuleFileName(NULL, winvncFolder, MAX_PATH))
	{
		char* p = strrchr(winvncFolder, '\\');
		*p = '\0';
	}
	extractConfig(szCmdLine2);
	InitCommonControls();
	INITCOMMONCONTROLSEX icex;
	memset(&icex, 0x0, sizeof(INITCOMMONCONTROLSEX));
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_COOL_CLASSES;
	InitCommonControlsEx(&icex);
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
		bool Injected_autoreconnect = false;
		settings->Initialize(configFile);
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
		strcpy_s(szCurrentDir, winvncFolder);
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

	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncStopserviceHelper, strlen(winvncStopserviceHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::Set_stop_service_as_admin();
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
					PostMessage(hservwnd, WM_COMMAND, ID_CLOSE_SILENT, 0);
					PostMessage(hservwnd, WM_CLOSE, 0, 0);
				}
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenhomepage, strlen(winvncopenhomepage)) == 0)
			{
				Open_homepage();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenforum, strlen(winvncopenforum)) == 0)
			{
				Open_forum();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopengithub, strlen(winvncopengithub)) == 0)
			{
				Open_github();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenmastodon, strlen(winvncopenmastodon)) == 0)
			{
				Open_mastodon();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenbluesky, strlen(winvncopenbluesky)) == 0)
			{
				Open_bluesky();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenfacebook, strlen(winvncopenfacebook)) == 0)
			{
				Open_facebook();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenxtwitter, strlen(winvncopenxtwitter)) == 0)
			{
				Open_xtwitter();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenreddit, strlen(winvncopenreddit)) == 0)
			{
				Open_reddit();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncopenopenhub, strlen(winvncopenopenhub)) == 0)
			{
				Open_openhub();
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
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncInstallServiceHelper, strlen(winvncInstallServiceHelper)) == 0)
			{
				//Sleeps are realy needed, else runas fails...
				Sleep(3000);
				serviceHelpers::Set_install_service_as_admin();
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncUnInstallServiceHelper, strlen(winvncUnInstallServiceHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::Set_uninstall_service_as_admin();
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncSoftwarecadHelper, strlen(winvncSoftwarecadHelper)) == 0)
			{
				Sleep(3000);
				vncCad::Enable_softwareCAD_elevated();
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncdelSoftwarecadHelper, strlen(winvncdelSoftwarecadHelper)) == 0)
			{
				Sleep(3000);
				vncCad::delete_softwareCAD_elevated();
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncRebootSafeHelper, strlen(winvncRebootSafeHelper)) == 0)
			{
				Sleep(3000);
				UltraVNCService::Reboot_in_safemode_elevated();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncRebootForceHelper, strlen(winvncRebootForceHelper)) == 0)
			{
				Sleep(3000);
				UltraVNCService::Reboot_with_force_reboot_elevated();
				return return2(0);
			}
			if (strncmp(&szCmdLine[i], winvncSecurityEditorHelper, strlen(winvncSecurityEditorHelper)) == 0)
			{
				Sleep(3000);
				serviceHelpers::winvncSecurityEditorHelper_as_admin();
				return return2(0);
			}
	#endif // SC_20
			if (strncmp(&szCmdLine[i], winvncSecurityEditor, strlen(winvncSecurityEditor)) == 0)
			{
				typedef void (*vncEditSecurityFn) (HWND hwnd, HINSTANCE hInstance);
				vncEditSecurityFn vncEditSecurity = 0;
				char szCurrentDir[MAX_PATH]{};
				strcpy_s(szCurrentDir, winvncFolder);
				strcat_s(szCurrentDir, "\\authSSP.dll");
				HMODULE hModule = LoadLibrary(szCurrentDir);
				if (hModule) {
					vncEditSecurity = (vncEditSecurityFn)GetProcAddress(hModule, "vncEditSecurity");
					CoInitialize(NULL);
					vncEditSecurity(NULL, hAppInstance);
					CoUninitialize();
					FreeLibrary(hModule);
				}
				return return2(0);
			}

	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncSoftwarecad, strlen(winvncSoftwarecad)) == 0)
			{
				vncCad::Enable_softwareCAD();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncdelSoftwarecad, strlen(winvncdelSoftwarecad)) == 0)
			{
				vncCad::delete_softwareCAD();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncRebootSafe, strlen(winvncRebootSafe)) == 0)
			{
				UltraVNCService::Reboot_in_safemode();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncRebootForce, strlen(winvncRebootForce)) == 0)
			{
				UltraVNCService::Reboot_with_force_reboot();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncStopservice, strlen(winvncStopservice)) == 0)
			{
				serviceHelpers::Real_stop_service();
				return return2(0);
			}

			if (strncmp(&szCmdLine[i], winvncStartservice, strlen(winvncStartservice)) == 0)
			{
				serviceHelpers::Real_start_service();
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
				return return2(0);
			}
	#endif // SC_20
			if (strncmp(&szCmdLine[i], winvncPreConnect, strlen(winvncPreConnect)) == 0)
			{
				i += strlen(winvncPreConnect);
				PreConnect = true;
				continue;
			}
			if (strncmp(&szCmdLine[i], winvncConfig, strlen(winvncConfig)) == 0) {
				i += strlen(winvncConfig);
				i += configfileskip; // was already extracted, just skip chars	
				continue;
			}
			if (strncmp(&szCmdLine[i], winvncRunService, strlen(winvncRunService)) == 0)
			{
				//Run as service
				if (!Myinit(hInstance)) return return2(0);
				settings->setRunningFromExternalService(true);
				int return2value = WinVNCAppMain();
				return return2(return2value);
			}

			if (strncmp(&szCmdLine[i], winvncRunServiceRdp, strlen(winvncRunServiceRdp)) == 0)
			{
				//Run as service
				if (!Myinit(hInstance)) return return2(0);
				settings->setRunningFromExternalService(true);
				settings->setRunningFromExternalServiceRdp(true);
				int return2value = WinVNCAppMain();
				return return2(return2value);
			}
	#ifndef SC_20
			if (strncmp(&szCmdLine[i], winvncStartService, strlen(winvncStartService)) == 0)
			{
				UltraVNCService::start_service(szCmdLine);
				return return2(0);
			}
	#endif // SC_20
			if (strncmp(&szCmdLine[i], winvncRunAsUserApp, strlen(winvncRunAsUserApp)) == 0)
			{
				// WinVNC is being run as a user-level program
				if (!Myinit(hInstance)) return return2(0);
				int return2value = WinVNCAppMain();
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
			
			if (strncmp(&szCmdLine[i], winvncsettings, strlen(winvncsettings)) == 0)
			{
				PropertiesDialog properties;
				properties.ShowDialog(true);
				PostQuitMessage(0);
			}

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
						if (settings->getIPV6()) {
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
					}
					else {
						VCard32 address = VSocket::Resolve(name);
#ifdef SC_20
						if (address == 0) {
							char text[1024]{};
							sprintf(text, " Hostnamee (%s) could not be resolved", name);
							helper::yesUVNCMessageBox(hInstResDLL, NULL, text, (char *)szAppName, MB_ICONEXCLAMATION);
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
					}
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
					if (settings->getIPV6()) {
						if (!postHelper::PostAddNewClient4(0, 0))
						{
							PostAddNewClient_bool = true;
							port_int = 0;
							memset(&address_in6, 0, sizeof(address_in6));
						}
					} 
					else {
						if (!postHelper::PostAddNewClient(0, 0))
						{
							PostAddNewClient_bool = true;
							port_int = 0;
							address_vcard = 0;
						}
					}
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
							if (settings->getIPV6()) {
								address_vcard4 = 0;
								memset(&address_in6, 0, sizeof(address_in6));
							}
							else 
								address_vcard = 0;
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
				return return2(0);
			int return2value = WinVNCAppMain();
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
					vnclog.Print(LL_INTERR, VNCLOG("GetUsername error %d\n"), GetLastError());
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
		if (settings->getIPV6()) 
			postHelper::PostAddNewClient6(&address_in6, port_int);
		else
			postHelper::PostAddNewClient(address_vcard, port_int);

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
