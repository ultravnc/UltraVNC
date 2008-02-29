// www.maxivista.com
// Special buil for Vista test
#include <windows.h>
#include <Userenv.h>
#include <WtsApi32.h>
#include <stdio.h>
#include <Tlhelp32.h>
#include "inifile.h"

HANDLE hEvent=NULL;
extern HANDLE stopServiceEvent;
static char app_path[MAX_PATH];
typedef DWORD (*WTSGETACTIVECONSOLESESSIONID)();
typedef bool (*WTSQUERYUSERTOKEN)(ULONG,PHANDLE);
WTSGETACTIVECONSOLESESSIONID lpfnWTSGetActiveConsoleSessionId=NULL;
WTSQUERYUSERTOKEN lpfnWTSQueryUserToken = NULL;
PROCESS_INFORMATION  ProcessInfo;
int counter=0;
extern char cmdtext[256];
int kickrdp=0;
bool W2K=0;
//////////////////////////////////////////////////////////////////////////////
static int pad2()
{

	OSVERSIONINFO OSversion;
	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);

	GetVersionEx(&OSversion);
	W2K=0;
	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
						  if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==0)
									 W2K=1;							    
								  
	}
	char exe_file_name[MAX_PATH];
	char cmdline[MAX_PATH];
    GetModuleFileName(0, exe_file_name, MAX_PATH);

    strcpy(app_path, exe_file_name);
	strcat(app_path, " ");
	strcat(app_path,cmdtext);
    strcat(app_path, "_run");
	IniFile myIniFile;
	kickrdp=myIniFile.ReadInt("admin", "kickrdp", kickrdp);
	myIniFile.ReadString("admin", "service_commandline",cmdline,256);
	if (strlen(cmdline)!=0)
	{
		strcpy(app_path, exe_file_name);
		strcat(app_path, " ");
		strcat(app_path,cmdline);
		strcat(app_path, " -service_run");
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////

BOOL SetTBCPrivileges(VOID) {
  DWORD dwPID;
  HANDLE hProcess;
  HANDLE hToken;
  LUID Luid;
  TOKEN_PRIVILEGES tpDebug;
  dwPID = GetCurrentProcessId();
  if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) == NULL) return FALSE;
  if (OpenProcessToken(hProcess, TOKEN_ALL_ACCESS, &hToken) == 0) return FALSE;
  if ((LookupPrivilegeValue(NULL, SE_TCB_NAME, &Luid)) == 0) return FALSE;
  tpDebug.PrivilegeCount = 1;
  tpDebug.Privileges[0].Luid = Luid;
  tpDebug.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  if ((AdjustTokenPrivileges(hToken, FALSE, &tpDebug, sizeof(tpDebug), NULL, NULL)) == 0) return FALSE;
  if (GetLastError() != ERROR_SUCCESS) return FALSE;
  CloseHandle(hToken);
  CloseHandle(hProcess);
  return TRUE;
}
//////////////////////////////////////////////////////////////////////////////
#include <tlhelp32.h>

DWORD GetwinlogonPid()
{
	DWORD dwSessionId;
	DWORD dwExplorerLogonPid=0;
	PROCESSENTRY32 procEntry;

	dwSessionId=0;

	

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE)
    {
        return 0 ;
    }

    procEntry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnap, &procEntry))
    {
		CloseHandle(hSnap);
        return 0 ;
    }

    do
    {
        if (_stricmp(procEntry.szExeFile, "winlogon.exe") == 0)
        {
			dwExplorerLogonPid = procEntry.th32ProcessID;
        }

    } while (Process32Next(hSnap, &procEntry));
	CloseHandle(hSnap);
	return dwExplorerLogonPid;
}
//////////////////////////////////////////////////////////////////////////////
DWORD
Find_winlogon(DWORD SessionId)
{

  PWTS_PROCESS_INFO pProcessInfo = NULL;
  DWORD         ProcessCount = 0;
//  char         szUserName[255];
  DWORD         Id = -1;

  if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &ProcessCount))
  {
    // dump each process description
    for (DWORD CurrentProcess = 0; CurrentProcess < ProcessCount; CurrentProcess++)
    {

        if( strcmp(pProcessInfo[CurrentProcess].pProcessName, "winlogon.exe") == 0 )
        {    
			if (SessionId==pProcessInfo[CurrentProcess].SessionId)
			{
            Id = pProcessInfo[CurrentProcess].ProcessId;
            break;
			}
        }
    }

    WTSFreeMemory(pProcessInfo);
  }

  return Id;
}

//////////////////////////////////////////////////////////////////////////////
BOOL
get_winlogon_handle(OUT LPHANDLE  lphUserToken)
{
	BOOL   bResult = FALSE;
	HANDLE hProcess;
	HANDLE hAccessToken = NULL;
	HANDLE hTokenThis = NULL;
	DWORD ID_session=0;
	if (lpfnWTSGetActiveConsoleSessionId!=NULL) ID_session=lpfnWTSGetActiveConsoleSessionId();
	DWORD Id=0;
	if (W2K==0) Id=Find_winlogon(ID_session);
	else Id=GetwinlogonPid();
	#ifdef _DEBUG
					char			szText[256];
					DWORD error=GetLastError();
					sprintf(szText," ++++++ Find_winlogon %i %i %d\n",ID_session,Id,error);
					SetLastError(0);
					OutputDebugString(szText);		
	#endif


	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, Id );
	if (hProcess)
	{
		#ifdef _DEBUG
					char			szText[256];
					DWORD error=GetLastError();
					sprintf(szText," ++++++ OpenProcess %i \n",hProcess);
					SetLastError(0);
					OutputDebugString(szText);		
		#endif

		OpenProcessToken(hProcess, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS, &hTokenThis);

		#ifdef _DEBUG
					error=GetLastError();
					sprintf(szText," ++++++ OpenProcessToken %i %i\n",hTokenThis,error);
					SetLastError(0);
					OutputDebugString(szText);		
		#endif
		{
		bResult = DuplicateTokenEx(hTokenThis, TOKEN_ASSIGN_PRIMARY|TOKEN_ALL_ACCESS,NULL, SecurityImpersonation, TokenPrimary, lphUserToken);
		#ifdef _DEBUG
					error=GetLastError();
					sprintf(szText," ++++++ DuplicateTokenEx %i %i %i %i\n",hTokenThis,&lphUserToken,error,bResult);
					SetLastError(0);
					OutputDebugString(szText);		
		#endif
		SetTokenInformation(*lphUserToken, TokenSessionId, &ID_session, sizeof(DWORD));
		#ifdef _DEBUG
					error=GetLastError();
					sprintf(szText," ++++++ SetTokenInformation( %i %i %i\n",hTokenThis,&lphUserToken,error);
					SetLastError(0);
					OutputDebugString(szText);		
		#endif
		CloseHandle(hTokenThis);
		}
	}
	return bResult;
}
//////////////////////////////////////////////////////////////////////////////

BOOL
GetSessionUserTokenWin(OUT LPHANDLE  lphUserToken)
{
  BOOL   bResult = FALSE;
  HANDLE hImpersonationToken = INVALID_HANDLE_VALUE;
  DWORD ID=0;
  if (lpfnWTSGetActiveConsoleSessionId!=NULL) ID=lpfnWTSGetActiveConsoleSessionId();
  HANDLE hTokenThis = NULL;
  
  if (lphUserToken != NULL) {   
		  bResult = get_winlogon_handle(lphUserToken);
  }
  return bResult;
}
//////////////////////////////////////////////////////////////////////////////
BOOL
GetSessionUserTokenDefault(OUT LPHANDLE  lphUserToken)
{
  BOOL   bResult = FALSE;
  HANDLE hImpersonationToken = INVALID_HANDLE_VALUE;
  DWORD ID=0;
  if (lpfnWTSGetActiveConsoleSessionId!=NULL) ID=lpfnWTSGetActiveConsoleSessionId();

  HANDLE hTokenThis = NULL;
  
  if (lphUserToken != NULL) {   
      if (lpfnWTSQueryUserToken (ID, &hImpersonationToken)) 
      {
        bResult = DuplicateTokenEx(hImpersonationToken,
                                   0,
                                   NULL,
                                   SecurityImpersonation,
                                   TokenPrimary,
                                   lphUserToken);
        CloseHandle(hImpersonationToken);
      }     
  }
  return bResult;

}
//////////////////////////////////////////////////////////////////////////////
// START the app as system 
BOOL
LaunchProcessWin()
{
  BOOL                 bReturn = FALSE;
  HANDLE               hToken;
  STARTUPINFO          StartUPInfo;
  PVOID                lpEnvironment = NULL;

  ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
  ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
  StartUPInfo.wShowWindow = SW_SHOW;
  StartUPInfo.lpDesktop = "Winsta0\\Winlogon";
  //StartUPInfo.lpDesktop = "Winsta0\\Default";
  StartUPInfo.cb = sizeof(STARTUPINFO);
  SetTBCPrivileges();
  pad2();

  if ( GetSessionUserTokenWin(&hToken) )
  {
      if ( CreateEnvironmentBlock(&lpEnvironment, hToken, FALSE) ) 
      {
      
		 SetLastError(0);
         if (CreateProcessAsUser(hToken,NULL,app_path,NULL,NULL,FALSE,CREATE_UNICODE_ENVIRONMENT |DETACHED_PROCESS,lpEnvironment,NULL,&StartUPInfo,&ProcessInfo))
			{
				counter=0;
				bReturn = TRUE;
				DWORD error=GetLastError();
				#ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++ CreateProcessAsUser winlogon %d\n",error);
					OutputDebugString(szText);		
				#endif
			}
		 else
		 {
			 DWORD error=GetLastError();
			 #ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++ CreateProcessAsUser failed %d %d %d\n",error,kickrdp,counter);
					OutputDebugString(szText);		
			#endif
			if (error==233 && kickrdp==1)
					{
						counter++;
						if (counter>3)
							{
								#ifdef _DEBUG
								DWORD error=GetLastError();
								sprintf(szText," ++++++ error==233 win\n");
								SetLastError(0);
								OutputDebugString(szText);		
								#endif
								typedef BOOLEAN (WINAPI * pWinStationConnect) (HANDLE,ULONG,ULONG,PCWSTR,ULONG);
								typedef BOOL (WINAPI * pLockWorkStation)();
								HMODULE  hlibwinsta = LoadLibrary("winsta.dll"); 
								HMODULE  hlibuser32 = LoadLibrary("user32.dll");
								pWinStationConnect WinStationConnectF=NULL;
								pLockWorkStation LockWorkStationF=NULL;

								if (hlibwinsta)
								   {
									   WinStationConnectF=(pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW"); 
								   }
								if (hlibuser32)
								   {
									   LockWorkStationF=(pLockWorkStation)GetProcAddress(hlibuser32, "LockWorkStation"); 
								   }
								if (WinStationConnectF!=NULL && WinStationConnectF!=NULL)
									{
											DWORD ID=0;
											if (lpfnWTSGetActiveConsoleSessionId!=NULL) ID=lpfnWTSGetActiveConsoleSessionId();
											WinStationConnectF(0, 0, ID, L"", 0);
											LockWorkStationF();
									}
								Sleep(3000);
						}
					}
		 }

         if (lpEnvironment) 
         {
            DestroyEnvironmentBlock(lpEnvironment);
         }

	  }//createenv
	  else
	  {
		  SetLastError(0);
         if (CreateProcessAsUser(hToken,NULL,app_path,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo))
			{
				counter=0;
				bReturn = TRUE;
				DWORD error=GetLastError();
				#ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++ CreateProcessAsUser winlogon %d\n",error);
					OutputDebugString(szText);		
				#endif
			}
		 else
		 {
			 DWORD error=GetLastError();
			 #ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++ CreateProcessAsUser no env failed %d\n",error);
					OutputDebugString(szText);		
			#endif
			//Little trick needed, FUS sometimes has an unreachable logon session.
			 //Switch to USER B, logout user B
			 //The logon session is then unreachable
			 //We force the logon session on the console
			if (error==233 && kickrdp==1)
					{
						counter++;
						if (counter>3)
							{
								#ifdef _DEBUG
								DWORD error=GetLastError();
								sprintf(szText," ++++++ error==233 win\n");
								SetLastError(0);
								OutputDebugString(szText);		
								#endif
								typedef BOOLEAN (WINAPI * pWinStationConnect) (HANDLE,ULONG,ULONG,PCWSTR,ULONG);
								typedef BOOL (WINAPI * pLockWorkStation)();
								HMODULE  hlibwinsta = LoadLibrary("winsta.dll"); 
								HMODULE  hlibuser32 = LoadLibrary("user32.dll");
								pWinStationConnect WinStationConnectF=NULL;
								pLockWorkStation LockWorkStationF=NULL;

								if (hlibwinsta)
								   {
									   WinStationConnectF=(pWinStationConnect)GetProcAddress(hlibwinsta, "WinStationConnectW"); 
								   }
								if (hlibuser32)
								   {
									   LockWorkStationF=(pLockWorkStation)GetProcAddress(hlibuser32, "LockWorkStation"); 
								   }
								if (WinStationConnectF!=NULL && WinStationConnectF!=NULL)
									{
											DWORD ID=0;
											if (lpfnWTSGetActiveConsoleSessionId!=NULL) ID=lpfnWTSGetActiveConsoleSessionId();
											WinStationConnectF(0, 0, ID, L"", 0);
											LockWorkStationF();
									}
								Sleep(3000);
						}
					}
	  }
        CloseHandle(hToken);
	}  //getsession
	}
  else
  {
	 #ifdef _DEBUG
	char			szText[256];
	DWORD error=GetLastError();
	sprintf(szText," ++++++ Getsessionusertokenwin failed %d\n",error);
	OutputDebugString(szText);		
	#endif

  }
    
    return bReturn;
}
//////////////////////////////////////////////////////////////////////////////
void monitor_sessions()
{
	pad2();
	HANDLE hTokenNew = NULL, hTokenDup = NULL;
	HMODULE  hmod = LoadLibrary("kernel32.dll");
	HMODULE  hmod2 = LoadLibrary("Wtsapi32.dll");

	lpfnWTSGetActiveConsoleSessionId = (WTSGETACTIVECONSOLESESSIONID)GetProcAddress(hmod,"WTSGetActiveConsoleSessionId"); 
	lpfnWTSQueryUserToken = (WTSQUERYUSERTOKEN)GetProcAddress(hmod2,"WTSQueryUserToken");

	DWORD dwSessionId=0;
	DWORD OlddwSessionId=99;
	ProcessInfo.hProcess=0;
	bool win=false;
	bool Slow_connect=false;
	bool last_con=false;
	//We use this event to notify the program that the session has changed
	//The program need to end so the service can restart the program in the correct session
	hEvent = CreateEvent(NULL, FALSE, FALSE, "Global\\SessionEventUltra");
	if (OlddwSessionId!=dwSessionId) SetEvent(hEvent);
	Sleep(3000);
	while(WaitForSingleObject(stopServiceEvent, 1000)==WAIT_TIMEOUT)
	{

		if (lpfnWTSGetActiveConsoleSessionId!=NULL)
		dwSessionId = lpfnWTSGetActiveConsoleSessionId();
		if (OlddwSessionId!=dwSessionId)
		{
			#ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++SetEvent \n");
					OutputDebugString(szText);		
			#endif
			SetEvent(hEvent);
		}

		
			

		if (dwSessionId!=0xFFFFFFFF)
			{
						DWORD dwCode=0;
						if (ProcessInfo.hProcess==NULL)
						{
									Sleep(1000);
									if (Slow_connect) Sleep(4000);
									LaunchProcessWin();
									win=false;
									Slow_connect=false;
						}
						else if (GetExitCodeProcess(ProcessInfo.hProcess,&dwCode))
						{
							if(dwCode != STILL_ACTIVE)
								{
									if (last_con==true)
									{
										//problems, we move from win-->default-->win
										// Put a long timeout to give system time to start or logout
										Sleep(10000);
									}
									Sleep(1000);
									if (Slow_connect) Sleep(4000);
									LaunchProcessWin();
									win=false;
									Slow_connect=false;
								}
							else
								{
									if (Slow_connect==false)
									{
										//This is the first time, so createprocess worked
										//last_con=false-->defaultdesk
										//last_con=true-->windesk
										last_con=false;
									}
									Slow_connect=true;
								}
						}
						else
						{
							if (last_con==true)
									{
										//problems, we move from win-->default-->win
										// Put a long timeout to give system time to start or logout
										Sleep(10000);
									}
							Sleep(1000);
							if (Slow_connect) Sleep(4000);
							LaunchProcessWin();
							win=false;
							Slow_connect=false;
						}
					#ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++1 %i %i %i %i\n",OlddwSessionId,dwSessionId,dwCode,ProcessInfo.hProcess);
					OutputDebugString(szText);		
					#endif
					OlddwSessionId=dwSessionId;
			}
	}//while
	#ifdef _DEBUG
					char			szText[256];
					sprintf(szText," ++++++SetEvent \n");
					OutputDebugString(szText);		
	#endif
	SetEvent(hEvent);
//	EndProcess();

}


