#include "cadthread.h"
#include "vncservice.h"
#include "localization.h"

vncCad::vncCad()
{
	
}

bool vncCad::ISUACENabled()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return false;
	HKEY hKey;
	if (::RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", &hKey) == ERROR_SUCCESS) {
		DWORD value = 0;
		DWORD tt=4;
		if (::RegQueryValueExW(hKey, L"EnableLUA", NULL, NULL, (LPBYTE)&value, &tt) == ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return (value != 0);
		}
		RegCloseKey(hKey);
	}
	return false;
}

void vncCad::Enable_softwareCAD()
{							
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
			0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS) 
			return;
		
	if (RegOpenKeyEx(hkLocal, "System", 0, KEY_WRITE | KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS) {
		RegCloseKey(hkLocal);
		return;
	}
	LONG pref;
	pref=1;
	RegSetValueEx(hkLocalKey, "SoftwareSASGeneration", 0, REG_DWORD, (LPBYTE) &pref, sizeof(pref));
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
}

void vncCad::delete_softwareCAD()
{
	//Beep(1000,10000);
	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)		
			return;		
	if (RegOpenKeyEx(hkLocal, "System", 0, KEY_WRITE | KEY_READ, &hkLocalKey) != ERROR_SUCCESS) {
		RegCloseKey(hkLocal);
		return;
	}
	RegDeleteValue(hkLocalKey, "SoftwareSASGeneration");
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);

}

void vncCad::delete_softwareCAD_elevated()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return;

	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-delsoftwarecad";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

void vncCad::Enable_softwareCAD_elevated()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion < 6) 
		return;
	char exe_file_name[MAX_PATH];
	GetModuleFileName(0, exe_file_name, MAX_PATH);
	SHELLEXECUTEINFO shExecInfo;
	shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	shExecInfo.fMask = NULL;
	shExecInfo.hwnd = GetForegroundWindow();
	shExecInfo.lpVerb = "runas";
	shExecInfo.lpFile = exe_file_name;
	shExecInfo.lpParameters = "-softwarecad";
	shExecInfo.lpDirectory = NULL;
	shExecInfo.nShow = SW_SHOWNORMAL;
	shExecInfo.hInstApp = NULL;
	ShellExecuteEx(&shExecInfo);
}

bool vncCad::IsSoftwareCadEnabled()
{
	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	if(OSversion.dwMajorVersion<6) return true;

	HKEY hkLocal, hkLocalKey;
	DWORD dw;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies",
		0, REG_NONE, REG_OPTION_NON_VOLATILE,
		KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
		{
		return 0;
		}
	if (RegOpenKeyEx(hkLocal,
		"System",
		0, KEY_READ,
		&hkLocalKey) != ERROR_SUCCESS)
	{
		RegCloseKey(hkLocal);
		return 0;
	}

	LONG pref=0;
	ULONG type = REG_DWORD;
	ULONG prefsize = sizeof(pref);

	if (RegQueryValueEx(hkLocalKey,
			"SoftwareSASGeneration",
			NULL,
			&type,
			(LPBYTE) &pref,
			&prefsize) != ERROR_SUCCESS)
	{
			RegCloseKey(hkLocalKey);
			RegCloseKey(hkLocal);
			return false;
	}
	RegCloseKey(hkLocalKey);
	RegCloseKey(hkLocal);
	if (pref!=0) return true;
	else return false;
}

DWORD WINAPI vncCad::Cadthread(LPVOID lpParam)
{
	//SET THREAD in CURRENT INPUTDESKTOP
	HANDLE	hShutdownEventcad;
	hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
	//Switch to InputDesktop
	HDESK desktop=NULL;
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);	
	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	SetThreadDesktop(desktop);

	OSVERSIONINFO OSversion;	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	//
	if(OSversion.dwMajorVersion>=6 && vncService::RunningAsService() && !IsSoftwareCadEnabled()) {				
		DWORD result=MessageBoxSecure(NULL,"UAC is Disable, make registry changes to allow cad","Warning",MB_YESNO);
		if (result==IDYES) {
			HANDLE hProcess=NULL,hPToken=NULL;
			DWORD id=vncService::GetExplorerLogonPid();
			if (id!=0)  {						
				hProcess = OpenProcess(MAXIMUM_ALLOWED,FALSE,id);
				if (!hProcess) 
					goto error;
				if(!OpenProcessToken(hProcess,TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY
						|TOKEN_DUPLICATE|TOKEN_ASSIGN_PRIMARY|TOKEN_ADJUST_SESSIONID
						| TOKEN_READ | TOKEN_WRITE, &hPToken)) {
					CloseHandle(hProcess);
					goto error;
				}
				char dir[MAX_PATH];
				char exe_file_name[MAX_PATH];
				GetModuleFileName(0, exe_file_name, MAX_PATH);
				strcpy(dir, exe_file_name);
				strcat(dir, " -softwarecadhelper");
								
				STARTUPINFO          StartUPInfo;
				PROCESS_INFORMATION  ProcessInfo;
				HANDLE Token=NULL;
				HANDLE process=NULL;
				ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
				ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
				StartUPInfo.wShowWindow = SW_SHOW;
				StartUPInfo.lpDesktop = "Winsta0\\Default";
				StartUPInfo.cb = sizeof(STARTUPINFO);
			
				CreateProcessAsUser(hPToken,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
				DWORD errorcode=GetLastError();
				if (process) CloseHandle(process);
				if (Token) CloseHandle(Token);
				if (ProcessInfo.hProcess) CloseHandle(ProcessInfo.hProcess);
				if (ProcessInfo.hThread) CloseHandle(ProcessInfo.hThread);
				if (errorcode == 1314) goto error;
					goto gotome;
			}
			error:
			Enable_softwareCAD_elevated();							
		}					
	}
	gotome:
	
    //Tell service to call sendSas()
	if(OSversion.dwMajorVersion >= 6)  {
		if (hShutdownEventcad == NULL )
			hShutdownEventcad = OpenEvent(EVENT_MODIFY_STATE, FALSE, "Global\\SessionEventUltraCad");
		if (hShutdownEventcad != NULL )
			SetEvent(hShutdownEventcad);
	}
	else { // call cad.exe
		char WORKDIR[MAX_PATH];
		char mycommand[MAX_PATH];
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH)) {
			char* p = strrchr(WORKDIR, '\\');
			if (p == NULL) return 0;
			*p = '\0';
		}
		strcpy_s(mycommand,"");
		strcat_s(mycommand,WORKDIR);//set the directory
		strcat_s(mycommand,"\\");
		strcat_s(mycommand,"cad.exe");
		ShellExecute(GetDesktopWindow(), "open", mycommand, "", 0, SW_SHOWNORMAL);
	}
	if (hShutdownEventcad)
		CloseHandle(hShutdownEventcad);
	if (old_desktop) 
		SetThreadDesktop(old_desktop);
	if (desktop) 
		CloseDesktop(desktop);
	return 0;
}