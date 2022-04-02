#pragma once
#ifndef SC_20
#define MAXSTRLENGTH    255

typedef struct _CPAU_PARAM {
	DWORD   cbSize;
	DWORD   dwProcessId;
	BOOL    bUseDefaultToken;
	HANDLE  hToken;
	LPWSTR  lpApplicationName;
	LPWSTR  lpCommandLine;
	SECURITY_ATTRIBUTES     ProcessAttributes;
	SECURITY_ATTRIBUTES ThreadAttributes;
	BOOL bInheritHandles;
	DWORD dwCreationFlags;
	LPVOID lpEnvironment;
	LPWSTR lpCurrentDirectory;
	STARTUPINFOW StartupInfo;
	PROCESS_INFORMATION     ProcessInformation;

}CPAU_PARAM;

typedef struct _CPAU_RET_PARAM {
	DWORD   cbSize;
	BOOL    bRetValue;
	DWORD   dwLastErr;
	PROCESS_INFORMATION     ProcInfo;

}CPAU_RET_PARAM;

typedef BOOLEAN(WINAPI* pWinStationQueryInformationW)(
	IN   HANDLE hServer,
	IN   ULONG LogonId,
	IN   DWORD /*WINSTATIONINFOCLASS*/ WinStationInformationClass,
	OUT  PVOID pWinStationInformation,
	IN   ULONG WinStationInformationLength,
	OUT  PULONG pReturnLength
	);

class UltraVNCService {
	
private:
	static void WINAPI service_main(DWORD argc, LPTSTR* argv);
	static void WINAPI control_handler(DWORD controlCode);
	static DWORD WINAPI control_handler_ex(DWORD controlCode, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
	static void set_service_description();
	static int pad();
	static void disconnect_remote_sessions();
	static bool IsAnyRDPSessionActive();

	static SERVICE_STATUS serviceStatus;
	static SERVICE_STATUS_HANDLE serviceStatusHandle;
	static char service_path[MAX_PATH];	
	static char* app_name;
	static char cmdtext[256];
	static char app_path[MAX_PATH];
	static void monitorSessions();
	static int kickrdp;
	static bool IsShutdown;
	static PROCESS_INFORMATION  ProcessInfo;
	static HANDLE hEndSessionEvent;
	static HANDLE hEvent;
	static int clear_console;

	static int createWinvncExeCall(bool preconnect, bool rdpselect);
	static BOOL LaunchProcessWin(DWORD dwSessionId, bool preconnect, bool rdpselect);
	static BOOL CreateRemoteSessionProcess(
			IN DWORD        dwSessionId, IN BOOL         bUseDefaultToken,IN HANDLE       hToken,IN LPCWSTR      lpApplicationName,
			IN LPSTR       A_lpCommandLine,IN LPSECURITY_ATTRIBUTES lpProcessAttributes,IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
			IN BOOL bInheritHandles,IN DWORD dwCreationFlags,IN LPVOID lpEnvironment,IN LPCWSTR lpCurrentDirectory,IN LPSTARTUPINFO A_lpStartupInfo, OUT LPPROCESS_INFORMATION lpProcessInformation);
	static DWORD MarshallString(LPCWSTR    pszText, LPVOID, DWORD  dwMaxSize, LPBYTE* ppNextBuf, DWORD* pdwUsedBytes);
	static BOOL Char2Wchar(WCHAR* pDest, char* pSrc, int nDestStrLen);

	static BOOL get_winlogon_handle(OUT LPHANDLE  lphUserToken, DWORD mysessionID);
	static BOOL GetSessionUserTokenWin(OUT LPHANDLE  lphUserToken, DWORD mysessionID);
	static DWORD GetwinlogonPid();
	static DWORD Find_winlogon(DWORD SessionId);
	static BOOL SetTBCPrivileges(VOID);
	static void wait_for_existing_process();
	static bool IsSessionStillActive(int ID);


public:
	UltraVNCService();
	static int start_service(char* cmd);
	static int install_service(void);
	static int uninstall_service(void);

	static BOOL CreateServiceSafeBootKey();	
	static void Set_Safemode();
	static BOOL reboot();
	static BOOL Force_reboot();
	static void Reboot_with_force_reboot();
	static void Reboot_with_force_reboot_elevated();
	static void Reboot_in_safemode();
	static void Reboot_in_safemode_elevated();
	static BOOL DeleteServiceSafeBootKey();
	static void Restore_safemode();
	static void Restore_after_reboot();

	static char service_name[256];
};
#endif
