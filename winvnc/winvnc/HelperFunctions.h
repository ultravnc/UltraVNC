#if (!defined(_HELPERFUNCTIONS))
#define _HELPERFUNCTIONS

#pragma once

#include <tlhelp32.h>
#include <string>

#define MSGFLT_ADD		1
extern bool ClientTimerReconnect;

extern HWND G_MENU_HWND;
extern char* MENU_CLASS_NAME;
extern bool allowMultipleInstances;

void Open_homepage();
void Open_forum();

namespace postHelper {
	extern UINT MENU_ADD_CLIENT_MSG;
	extern UINT MENU_REPEATER_ID_MSG;
	extern UINT MENU_AUTO_RECONNECT_MSG;
	extern UINT MENU_STOP_RECONNECT_MSG;
	extern UINT MENU_STOP_ALL_RECONNECT_MSG;
	extern UINT MENU_ADD_CLIENT_MSG_INIT;
	extern UINT MENU_ADD_CLOUD_MSG;
	extern UINT MENU_ADD_CLIENT6_MSG_INIT;
	extern UINT MENU_ADD_CLIENT6_MSG;
	extern UINT MENU_TRAYICON_BALLOON_MSG;
	extern UINT FileTransferSendPacketMessage;
	extern in6_addr G_LPARAM_IN6;

	BOOL PostAddAutoConnectClient(const char* pszId);
	BOOL PostAddNewRepeaterClient();
	BOOL PostAddNewCloudClient();
	BOOL PostAddStopConnectClient();
	BOOL PostAddStopConnectClientAll();
	BOOL PostAddNewClientInit(unsigned long ipaddress, unsigned short port);

	BOOL PostAddNewClient4(unsigned long ipaddress, unsigned short port);
	BOOL PostAddNewClientInit4(unsigned long ipaddress, unsigned short port);
	BOOL PostAddNewClient6(in6_addr* ipaddress, unsigned short port);
	BOOL PostAddNewClientInit6(in6_addr* ipaddress, unsigned short port);

	BOOL PostAddNewClient(unsigned long ipaddress, unsigned short port);
	BOOL PostAddConnectClient(const char* pszId);
	BOOL PostToThisWinVNC(UINT message, WPARAM wParam, LPARAM lParam);
	BOOL PostToWinVNC(UINT message, WPARAM wParam, LPARAM lParam);

	HWND FindWinVNCWindow(bool bThisProcess);
}

namespace processHelper {
	BOOL CurrentUser(char* buffer, UINT size);
	DWORD GetExplorerLogonPid();
	bool IsServiceInstalled();
	DWORD GetCurrentConsoleSessionID();
	BOOL IsWSLocked();
}

namespace desktopSelector {
	int InputDesktopSelected();
	BOOL SelectHDESK(HDESK new_desktop);
	BOOL SelectDesktop(char* name, HDESK* new_desktop);
}

#ifndef SC_20
namespace serviceHelpers {
	//bool ExistServiceName(TCHAR* pszAppPath, TCHAR* pszServiceName);
	void make_upper(std::string& str);
	void winvncSecurityEditorHelper_as_admin();
	void Set_uninstall_service_as_admin();
	void Set_install_service_as_admin();
	void Real_start_service();
	void Set_start_service_as_admin();
	void Real_stop_service();
	void Set_stop_service_as_admin();
}
#endif

namespace settingsHelpers {
	void Set_settings_as_admin(char* mycommand);
	void Real_settings(char* mycommand);
	void Copy_to_Secure_from_temp_helper(char* lpCmdLine);
}

DWORD MessageBoxSecure(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);

#endif