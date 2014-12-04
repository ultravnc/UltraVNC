// uvnc_settings.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "uvnc_settings.h"
#include <commctrl.h>
#include "inifile.h"
#include "resource.h"

#define MAX_LOADSTRING 100
#define MAXPWLEN 8

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc(HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOptions1(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK security(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOFT(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcCON(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcCAP(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMISC(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcService(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcSFX2(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam);
HWND m_hParent;
HWND m_hTab;
HWND hTab0dialog,hTab1dialog,hTab2dialog,hTab3dialog,hTab4dialog,hTab5dialog,hTab6dialog;//,hTab7dialog,hTab8dialog;

/*TCHAR *group1=new char[150];
TCHAR *group2=new char[150];
TCHAR *group3=new char[150];*/
LONG BUseRegistry=0;
LONG MSLogonRequired=0;
LONG NewMSLogon=0;
/*LONG locdom1;
LONG locdom2;
LONG locdom3;*/

LONG DebugMode=2;
LONG Avilog=0;
LONG DebugLevel=10;
LONG DisableTrayIcon=0;
LONG LoopbackOnly=0;
LONG UseDSMPlugin=0;
LONG AllowLoopback=1;
LONG AuthRequired=1;
LONG ConnectPriority=0;

char DSMPlugin[128];
char authhosts[150]= ("\0");

LONG AllowShutdown=1;
LONG AllowProperties=1;
LONG AllowEditClients=1;

LONG FileTransferEnabled=1;
LONG FTUserImpersonation=1;
LONG BlankMonitorEnabled=1;
LONG BlankInputsOnly=0; //PGM
LONG DefaultScale=1;
LONG FTTimeout = 30;

LONG SocketConnect=1;
LONG HTTPConnect=1;
LONG AutoPortSelect=1;
LONG PortNumber=5900;
LONG HttpPortNumber=5800;
LONG IdleTimeout=0;
LONG IdleInputTimeout=0;

LONG RemoveWallpaper=0;
LONG RemoveAero=0;

LONG QuerySetting=1;
LONG QueryTimeout=10;
LONG QueryAccept=0;
LONG QueryIfNoLogon=0;

LONG EnableRemoteInputs=1;
LONG LockSettings=0;
LONG DisableLocalInputs=0;
LONG EnableJapInput=0;
LONG kickrdp=0;


char passwd[MAXPWLEN];
char passwd2[MAXPWLEN]; //PGM

LONG TurboMode=1;
LONG PollUnderCursor=0;
LONG PollForeground=0;
LONG PollFullScreen=1;
LONG PollConsoleOnly=0;
LONG PollOnEventOnly=0;
LONG Driver=0;
LONG Hook=1;
LONG Virtual=0;
LONG SingleWindow=0;
char SingleWindowName[32];
char path[512];
LONG Primary=1;
LONG Secondary=0;
char servicecmdline[256]=("\0");

char accept_reject_mesg[512];
LONG MaxCpu=40;
LONG clearconsole=0;

LONG keepAliveInterval=5;



int vncEncryptPasswd(char *passwd, char *encryptedPasswd);
INT_PTR CALLBACK PasswdProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			char plaintext1[MAXPWLEN+1]; //PGM
			int len = GetDlgItemText(hDlg, IDC_SFXPASSWD, (LPSTR) &plaintext1, MAXPWLEN+1); //PGM
			if (len == 0)
					{
						strcpy(passwd,"");
					}
			else
					{
						vncEncryptPasswd(plaintext1,passwd); //PGM
					}

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void Save_settings()
{
IniFile myIniFile_Out;
myIniFile_Out.IniFileSetSecure();
myIniFile_Out.WriteInt("admin", "UseRegistry", BUseRegistry);
myIniFile_Out.WriteInt("admin", "MSLogonRequired", MSLogonRequired);
myIniFile_Out.WriteInt("admin", "NewMSLogon", NewMSLogon);
myIniFile_Out.WriteInt("admin", "DebugMode", DebugMode);
myIniFile_Out.WriteInt("admin", "Avilog", Avilog);
myIniFile_Out.WriteString("admin", "path", path);

myIniFile_Out.WriteInt("admin", "kickrdp", kickrdp);
myIniFile_Out.WriteString("admin", "service_commandline", servicecmdline);

myIniFile_Out.WriteInt("admin", "DebugLevel", DebugLevel);
myIniFile_Out.WriteInt("admin", "DisableTrayIcon", DisableTrayIcon);
myIniFile_Out.WriteInt("admin", "LoopbackOnly", LoopbackOnly);
myIniFile_Out.WriteInt("admin", "UseDSMPlugin", UseDSMPlugin);
myIniFile_Out.WriteInt("admin", "AllowLoopback", AllowLoopback);
myIniFile_Out.WriteInt("admin", "AuthRequired", AuthRequired);
myIniFile_Out.WriteInt("admin", "ConnectPriority", ConnectPriority);
myIniFile_Out.WriteString("admin", "DSMPlugin",DSMPlugin);
myIniFile_Out.WriteString("admin", "AuthHosts",authhosts);
myIniFile_Out.WriteInt("admin", "AllowShutdown" ,AllowShutdown);
myIniFile_Out.WriteInt("admin", "AllowProperties" ,AllowProperties);
myIniFile_Out.WriteInt("admin", "AllowEditClients" ,AllowEditClients);
myIniFile_Out.WriteInt("admin", "FileTransferEnabled", FileTransferEnabled);
myIniFile_Out.WriteInt("admin", "FTUserImpersonation", FTUserImpersonation);
myIniFile_Out.WriteInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
myIniFile_Out.WriteInt("admin", "BlankInputsOnly", BlankInputsOnly); //PGM
myIniFile_Out.WriteInt("admin", "DefaultScale", DefaultScale);
myIniFile_Out.WriteInt("admin", "SocketConnect", SocketConnect);
myIniFile_Out.WriteInt("admin", "HTTPConnect", HTTPConnect);
myIniFile_Out.WriteInt("admin", "AutoPortSelect", AutoPortSelect);
myIniFile_Out.WriteInt("admin", "PortNumber", PortNumber);
myIniFile_Out.WriteInt("admin", "HTTPPortNumber", HttpPortNumber);
myIniFile_Out.WriteInt("admin", "IdleTimeout", IdleTimeout);
myIniFile_Out.WriteInt("admin", "IdleInputTimeout", IdleInputTimeout);
myIniFile_Out.WriteInt("admin", "RemoveWallpaper", RemoveWallpaper);
myIniFile_Out.WriteInt("admin", "RemoveAero", RemoveAero);
myIniFile_Out.WriteInt("admin", "QuerySetting", QuerySetting);
myIniFile_Out.WriteInt("admin", "QueryTimeout", QueryTimeout);
myIniFile_Out.WriteInt("admin", "QueryAccept", QueryAccept);
myIniFile_Out.WriteInt("admin", "QueryIfNoLogon", QueryIfNoLogon);

myIniFile_Out.WriteInt("admin", "primary", Primary);
myIniFile_Out.WriteInt("admin", "secondary", Secondary);


myIniFile_Out.WritePassword(passwd);
myIniFile_Out.WritePassword2(passwd2); //PGM
myIniFile_Out.WriteInt("admin", "InputsEnabled", EnableRemoteInputs);
myIniFile_Out.WriteInt("admin", "LockSetting", LockSettings);
myIniFile_Out.WriteInt("admin", "LocalInputsDisabled", DisableLocalInputs);	
myIniFile_Out.WriteInt("admin", "EnableJapInput", EnableJapInput);	
myIniFile_Out.WriteInt("poll", "TurboMode", TurboMode);
myIniFile_Out.WriteInt("poll", "PollUnderCursor", PollUnderCursor);
myIniFile_Out.WriteInt("poll", "PollForeground", PollForeground);
myIniFile_Out.WriteInt("poll", "PollFullScreen", PollFullScreen);
myIniFile_Out.WriteInt("poll", "OnlyPollConsole",PollConsoleOnly);
myIniFile_Out.WriteInt("poll", "OnlyPollOnEvent", PollOnEventOnly);
myIniFile_Out.WriteInt("poll", "EnableDriver", Driver);
myIniFile_Out.WriteInt("poll", "EnableHook", Hook);
myIniFile_Out.WriteInt("poll", "EnableVirtual", Virtual);
myIniFile_Out.WriteInt("poll", "SingleWindow", SingleWindow);
myIniFile_Out.WriteString("poll", "SingleWindowName", SingleWindowName);
myIniFile_Out.WriteInt("admin", "FileTransferTimeout", FTTimeout);

myIniFile_Out.WriteInt("admin", "clearconsole", clearconsole);
myIniFile_Out.WriteInt("poll", "MaxCpu", MaxCpu);
myIniFile_Out.WriteString("admin", "accept_reject_mesg", accept_reject_mesg);
myIniFile_Out.WriteInt("admin", "KeepAliveInterval", keepAliveInterval);
}

void Save_settings_sfx(char *myfile)
{
IniFile myIniFile_Out;
myIniFile_Out.IniFileSetTemp(myfile);
myIniFile_Out.WriteInt("admin", "UseRegistry", BUseRegistry);
myIniFile_Out.WriteInt("admin", "MSLogonRequired", MSLogonRequired);
myIniFile_Out.WriteInt("admin", "NewMSLogon", NewMSLogon);
myIniFile_Out.WriteInt("admin", "DebugMode", DebugMode);
myIniFile_Out.WriteInt("admin", "Avilog", Avilog);
myIniFile_Out.WriteString("admin", "path", path);

myIniFile_Out.WriteInt("admin", "kickrdp", kickrdp);
myIniFile_Out.WriteString("admin", "service_commandline", servicecmdline);

myIniFile_Out.WriteInt("admin", "DebugLevel", DebugLevel);
myIniFile_Out.WriteInt("admin", "DisableTrayIcon", DisableTrayIcon);
myIniFile_Out.WriteInt("admin", "LoopbackOnly", LoopbackOnly);
myIniFile_Out.WriteInt("admin", "UseDSMPlugin", UseDSMPlugin);
myIniFile_Out.WriteInt("admin", "AllowLoopback", AllowLoopback);
myIniFile_Out.WriteInt("admin", "AuthRequired", AuthRequired);
myIniFile_Out.WriteInt("admin", "ConnectPriority", ConnectPriority);
myIniFile_Out.WriteString("admin", "DSMPlugin",DSMPlugin);
myIniFile_Out.WriteString("admin", "AuthHosts",authhosts);
myIniFile_Out.WriteInt("admin", "AllowShutdown" ,AllowShutdown);
myIniFile_Out.WriteInt("admin", "AllowProperties" ,AllowProperties);
myIniFile_Out.WriteInt("admin", "AllowEditClients" ,AllowEditClients);
myIniFile_Out.WriteInt("admin", "FileTransferEnabled", FileTransferEnabled);
myIniFile_Out.WriteInt("admin", "FTUserImpersonation", FTUserImpersonation);
myIniFile_Out.WriteInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
myIniFile_Out.WriteInt("admin", "BlankInputsOnly", BlankInputsOnly); //PGM
myIniFile_Out.WriteInt("admin", "DefaultScale", DefaultScale);
myIniFile_Out.WriteInt("admin", "SocketConnect", SocketConnect);
myIniFile_Out.WriteInt("admin", "HTTPConnect", HTTPConnect);
myIniFile_Out.WriteInt("admin", "AutoPortSelect", AutoPortSelect);
myIniFile_Out.WriteInt("admin", "PortNumber", PortNumber);
myIniFile_Out.WriteInt("admin", "HTTPPortNumber", HttpPortNumber);
myIniFile_Out.WriteInt("admin", "IdleTimeout", IdleTimeout);
myIniFile_Out.WriteInt("admin", "IdleInputTimeout", IdleInputTimeout);
myIniFile_Out.WriteInt("admin", "RemoveWallpaper", RemoveWallpaper);
myIniFile_Out.WriteInt("admin", "RemoveAero", RemoveAero);
myIniFile_Out.WriteInt("admin", "QuerySetting", QuerySetting);
myIniFile_Out.WriteInt("admin", "QueryTimeout", QueryTimeout);
myIniFile_Out.WriteInt("admin", "QueryAccept", QueryAccept);
myIniFile_Out.WriteInt("admin", "QueryIfNoLogon", QueryIfNoLogon);

myIniFile_Out.WriteInt("admin", "primary", Primary);
myIniFile_Out.WriteInt("admin", "secondary", Secondary);

DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PASSWD), 
							NULL, (DLGPROC) PasswdProc, NULL);
myIniFile_Out.WritePassword(passwd);
myIniFile_Out.WritePassword2(passwd2); //PGM

myIniFile_Out.WriteInt("admin", "InputsEnabled", EnableRemoteInputs);
myIniFile_Out.WriteInt("admin", "LockSetting", LockSettings);
myIniFile_Out.WriteInt("admin", "LocalInputsDisabled", DisableLocalInputs);	
myIniFile_Out.WriteInt("admin", "EnableJapInput", EnableJapInput);	
myIniFile_Out.WriteInt("poll", "TurboMode", TurboMode);
myIniFile_Out.WriteInt("poll", "PollUnderCursor", PollUnderCursor);
myIniFile_Out.WriteInt("poll", "PollForeground", PollForeground);
myIniFile_Out.WriteInt("poll", "PollFullScreen", PollFullScreen);
myIniFile_Out.WriteInt("poll", "OnlyPollConsole",PollConsoleOnly);
myIniFile_Out.WriteInt("poll", "OnlyPollOnEvent", PollOnEventOnly);
myIniFile_Out.WriteInt("poll", "EnableDriver", Driver);
myIniFile_Out.WriteInt("poll", "EnableHook", Hook);
myIniFile_Out.WriteInt("poll", "EnableVirtual", Virtual);
myIniFile_Out.WriteInt("poll", "SingleWindow", SingleWindow);
myIniFile_Out.WriteString("poll", "SingleWindowName", SingleWindowName);
myIniFile_Out.WriteInt("admin", "FileTransferTimeout", FTTimeout);
myIniFile_Out.WriteInt("admin", "clearconsole", clearconsole);
myIniFile_Out.WriteInt("poll", "MaxCpu", MaxCpu);
myIniFile_Out.WriteString("admin", "accept_reject_mesg", accept_reject_mesg);
}

void Read_Settings_from_ini()
{
IniFile myIniFile_In;
kickrdp=myIniFile_In.ReadInt("admin", "kickrdp", kickrdp);
myIniFile_In.ReadString("admin", "service_commandline",servicecmdline,256);
myIniFile_In.IniFileSetSecure();
BUseRegistry = myIniFile_In.ReadInt("admin", "UseRegistry", 0);
MSLogonRequired=myIniFile_In.ReadInt("admin", "MSLogonRequired", MSLogonRequired);
NewMSLogon=myIniFile_In.ReadInt("admin", "NewMSLogon", NewMSLogon);
/*myIniFile_In.ReadString("admin_auth","group1",group1,150);
myIniFile_In.ReadString("admin_auth","group2",group2,150);
myIniFile_In.ReadString("admin_auth","group3",group3,150);
locdom1=myIniFile_In.ReadInt("admin_auth", "locdom1",0);
locdom2=myIniFile_In.ReadInt("admin_auth", "locdom2",0);
locdom3=myIniFile_In.ReadInt("admin_auth", "locdom3",0);*/
DebugMode=myIniFile_In.ReadInt("admin", "DebugMode",DebugMode);
Avilog=myIniFile_In.ReadInt("admin", "Avilog", Avilog);
myIniFile_In.ReadString("admin", "path", path,512);
DebugLevel=myIniFile_In.ReadInt("admin", "DebugLevel", DebugLevel);
DisableTrayIcon=myIniFile_In.ReadInt("admin", "DisableTrayIcon", DisableTrayIcon);
LoopbackOnly=myIniFile_In.ReadInt("admin", "LoopbackOnly", LoopbackOnly);
UseDSMPlugin=myIniFile_In.ReadInt("admin", "UseDSMPlugin", UseDSMPlugin);
AllowLoopback=myIniFile_In.ReadInt("admin", "AllowLoopback", AllowLoopback);
AuthRequired=myIniFile_In.ReadInt("admin", "AuthRequired", AuthRequired);
ConnectPriority=myIniFile_In.ReadInt("admin", "ConnectPriority",ConnectPriority);
myIniFile_In.ReadString("admin", "DSMPlugin",DSMPlugin,128);
myIniFile_In.ReadString("admin", "AuthHosts",authhosts,150);
AllowShutdown=myIniFile_In.ReadInt("admin", "AllowShutdown", AllowShutdown);
AllowProperties=myIniFile_In.ReadInt("admin", "AllowProperties", AllowProperties);
AllowEditClients=myIniFile_In.ReadInt("admin", "AllowEditClients", AllowEditClients);
FileTransferEnabled=myIniFile_In.ReadInt("admin", "FileTransferEnabled", FileTransferEnabled);
FTUserImpersonation=myIniFile_In.ReadInt("admin", "FTUserImpersonation", FTUserImpersonation);
BlankMonitorEnabled = myIniFile_In.ReadInt("admin", "BlankMonitorEnabled", BlankMonitorEnabled);
BlankInputsOnly = myIniFile_In.ReadInt("admin", "BlankInputsOnly", BlankInputsOnly); //PGM
DefaultScale = myIniFile_In.ReadInt("admin", "DefaultScale", DefaultScale);
FTTimeout = myIniFile_In.ReadInt("admin", "FileTransferTimeout", DefaultScale);
SocketConnect=myIniFile_In.ReadInt("admin", "SocketConnect", SocketConnect);
HTTPConnect=myIniFile_In.ReadInt("admin", "HTTPConnect", HTTPConnect);
AutoPortSelect=myIniFile_In.ReadInt("admin", "AutoPortSelect", AutoPortSelect);
PortNumber=myIniFile_In.ReadInt("admin", "PortNumber", PortNumber);
HttpPortNumber=myIniFile_In.ReadInt("admin", "HTTPPortNumber",HttpPortNumber);
IdleTimeout=myIniFile_In.ReadInt("admin", "IdleTimeout", IdleTimeout);	
IdleInputTimeout=myIniFile_In.ReadInt("admin", "IdleInputTimeout", IdleInputTimeout);
RemoveWallpaper=myIniFile_In.ReadInt("admin", "RemoveWallpaper", RemoveWallpaper);
RemoveAero=myIniFile_In.ReadInt("admin", "RemoveAero", RemoveAero);
QuerySetting=myIniFile_In.ReadInt("admin", "QuerySetting", QuerySetting);
QueryTimeout=myIniFile_In.ReadInt("admin", "QueryTimeout", QueryTimeout);
QueryAccept=myIniFile_In.ReadInt("admin", "QueryAccept", QueryAccept);
QueryIfNoLogon=myIniFile_In.ReadInt("admin", "QueryIfNoLogon", QueryIfNoLogon);
myIniFile_In.ReadPassword(passwd,MAXPWLEN);
myIniFile_In.ReadPassword2(passwd2,MAXPWLEN); //PGM
EnableRemoteInputs=myIniFile_In.ReadInt("admin", "InputsEnabled", EnableRemoteInputs);
LockSettings=myIniFile_In.ReadInt("admin", "LockSetting",LockSettings);
DisableLocalInputs=myIniFile_In.ReadInt("admin", "LocalInputsDisabled", DisableLocalInputs);
EnableJapInput=myIniFile_In.ReadInt("admin", "EnableJapInput", EnableJapInput);

Primary=myIniFile_In.ReadInt("admin", "primary", Primary);
Secondary=myIniFile_In.ReadInt("admin", "secondary", Secondary);

TurboMode = myIniFile_In.ReadInt("poll", "TurboMode", TurboMode);
PollUnderCursor=myIniFile_In.ReadInt("poll", "PollUnderCursor", PollUnderCursor);
PollForeground=myIniFile_In.ReadInt("poll", "PollForeground", PollForeground);
PollFullScreen=myIniFile_In.ReadInt("poll", "PollFullScreen", PollFullScreen);
PollConsoleOnly=myIniFile_In.ReadInt("poll", "OnlyPollConsole",PollConsoleOnly);
PollOnEventOnly=myIniFile_In.ReadInt("poll", "OnlyPollOnEvent", PollOnEventOnly);
Driver=myIniFile_In.ReadInt("poll", "EnableDriver", 0);
Hook=myIniFile_In.ReadInt("poll", "EnableHook", Hook);
Virtual=myIniFile_In.ReadInt("poll", "EnableVirtual", Virtual);
SingleWindow=myIniFile_In.ReadInt("poll","SingleWindow",SingleWindow);
myIniFile_In.ReadString("poll", "SingleWindowName", SingleWindowName,32);

clearconsole=myIniFile_In.ReadInt("admin", "clearconsole", clearconsole);
MaxCpu=myIniFile_In.ReadInt("poll", "MaxCpu", MaxCpu);
myIniFile_In.ReadString("admin", "accept_reject_mesg", accept_reject_mesg,512);

keepAliveInterval = myIniFile_In.ReadInt("admin", "KeepAliveInterval", keepAliveInterval); 
}

DWORD WINAPI upnpthread( LPVOID lpParam );
int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles. Otherwise, any window creation will fail.
	InitCommonControls(); 
	INITCOMMONCONTROLSEX icex;
    memset(&icex, 0x0, sizeof(INITCOMMONCONTROLSEX));
    icex.dwSize = sizeof (INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_COOL_CLASSES;
    InitCommonControlsEx (&icex);
	


	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Initialize global strings
	hInst=hInstance;
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_UVNC_SETTINGS, szWindowClass, MAX_LOADSTRING);
	Read_Settings_from_ini();
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_PARENT), 
							NULL, (DLGPROC) DlgProc, NULL);
	
}


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

BOOL CALLBACK DlgProc(HWND hwndDlg, UINT uMsg,
										WPARAM wParam, LPARAM lParam)
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
//	VNCOptions *_this = (VNCOptions *) GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (uMsg) {
	case WM_INITDIALOG: 
		{
			InitCommonControls();
			m_hParent = hwndDlg;
			m_hTab = GetDlgItem(hwndDlg, IDC_TAB);

			TCITEM item;
			item.mask = TCIF_TEXT; 
			item.pszText="Network";
			TabCtrl_InsertItem(m_hTab, 0, &item);
			item.pszText = "Security";
			TabCtrl_InsertItem(m_hTab, 1, &item);
			item.pszText = "Input/FileTransfer";
			TabCtrl_InsertItem(m_hTab, 2, &item);
			item.pszText = "Connections";
			TabCtrl_InsertItem(m_hTab, 3, &item);
			item.pszText = "Screen Capture";
			TabCtrl_InsertItem(m_hTab, 4, &item);
			item.pszText = "Misc/logging";
			TabCtrl_InsertItem(m_hTab, 5, &item);
			item.pszText = "Service";
			TabCtrl_InsertItem(m_hTab, 6, &item);

			hTab0dialog = CreateDialogParam(hInst,
				MAKEINTRESOURCE(IDD_OPTION1),
				hwndDlg,
				(DLGPROC)DlgProcOptions1,
				NULL);

			hTab1dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION2),
				hwndDlg,
				(DLGPROC)security,
				NULL);

			hTab2dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION3),
				hwndDlg,
				(DLGPROC)DlgProcOFT,
				NULL);
			hTab3dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION4),
				hwndDlg,
				(DLGPROC)DlgProcCON,
				NULL);

			hTab4dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION5),
				hwndDlg,
				(DLGPROC)DlgProcCAP,
				NULL);

			hTab5dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION7),
				hwndDlg,
				(DLGPROC)DlgProcMISC,
				NULL);

			hTab6dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION8),
				hwndDlg,
				(DLGPROC)DlgProcService,
				NULL);

			/*hTab7dialog = CreateDialogParam(hInst, 
				MAKEINTRESOURCE(IDD_OPTION9),
				hwndDlg,
				(DLGPROC)DlgProcSFX2,
				NULL);*/

			// Position child dialogs, to fit the Tab control's display area
			RECT rc;
			GetWindowRect(m_hTab, &rc);
			MapWindowPoints(NULL, hwndDlg, (POINT *)&rc, 2);
			TabCtrl_AdjustRect(m_hTab, FALSE, &rc);
			SetWindowPos(hTab0dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_SHOWWINDOW);
			SetWindowPos(hTab1dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);
			SetWindowPos(hTab2dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);
			SetWindowPos(hTab3dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);
			SetWindowPos(hTab4dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);
			SetWindowPos(hTab5dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);
			SetWindowPos(hTab6dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);
			/*SetWindowPos(hTab7dialog, HWND_TOP, rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top,
						 SWP_HIDEWINDOW);*/
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))	{

		case IDC_HELP2:
			{
			int i = TabCtrl_GetCurFocus(m_hTab);
			SendMessage(hTab0dialog, WM_COMMAND, IDC_HELP2, i);
			SendMessage(hTab1dialog, WM_COMMAND, IDC_HELP2, i);
			SendMessage(hTab2dialog, WM_COMMAND, IDC_HELP2, i);
			SendMessage(hTab3dialog, WM_COMMAND, IDC_HELP2, i);
			SendMessage(hTab4dialog, WM_COMMAND, IDC_HELP2, i);
			SendMessage(hTab5dialog, WM_COMMAND, IDC_HELP2, i);
			SendMessage(hTab6dialog, WM_COMMAND, IDC_HELP2, i);	
			}
			break;
		case IDOK:
			///SetFocus(GetDlgItem(hwndDlg, IDOK));
			SendMessage(hTab0dialog, WM_COMMAND, IDOK, 0);
			SendMessage(hTab1dialog, WM_COMMAND, IDOK, 0);
			SendMessage(hTab2dialog, WM_COMMAND, IDOK, 0);
			SendMessage(hTab3dialog, WM_COMMAND, IDOK, 0);
			SendMessage(hTab4dialog, WM_COMMAND, IDOK, 0);
			SendMessage(hTab5dialog, WM_COMMAND, IDOK, 0);
			SendMessage(hTab6dialog, WM_COMMAND, IDOK, 0);	
			//SendMessage(hTab7dialog, WM_COMMAND, IDOK, 0);	
			//SendMessage(_this->m_hPageGeneral, WM_COMMAND, IDC_OK, 0);
			DestroyWindow(hTab0dialog);
			DestroyWindow(hTab1dialog);
			DestroyWindow(hTab2dialog);
			DestroyWindow(hTab3dialog);
			DestroyWindow(hTab4dialog);
			DestroyWindow(hTab5dialog);
			DestroyWindow(hTab6dialog);
			//DestroyWindow(hTab7dialog);
			Save_settings();
			EndDialog(hwndDlg, TRUE);
			return TRUE;
		case IDCANCEL:			
			SendMessage(hTab0dialog, WM_COMMAND, IDCANCEL, 0);
			SendMessage(hTab1dialog, WM_COMMAND, IDCANCEL, 0);
			SendMessage(hTab2dialog, WM_COMMAND, IDCANCEL, 0);
			SendMessage(hTab3dialog, WM_COMMAND, IDCANCEL, 0);
			SendMessage(hTab4dialog, WM_COMMAND, IDCANCEL, 0);
			SendMessage(hTab5dialog, WM_COMMAND, IDCANCEL, 0);
			SendMessage(hTab6dialog, WM_COMMAND, IDCANCEL, 0);
			//SendMessage(hTab7dialog, WM_COMMAND, IDCANCEL, 0);
			
			DestroyWindow(hTab0dialog);
			DestroyWindow(hTab1dialog);
			DestroyWindow(hTab2dialog);
			DestroyWindow(hTab3dialog);
			DestroyWindow(hTab4dialog);
			DestroyWindow(hTab5dialog);
			DestroyWindow(hTab6dialog);
			//DestroyWindow(hTab7dialog);
			
			EndDialog(hwndDlg, FALSE);
			return TRUE;
		}
		return FALSE;

	case WM_NOTIFY:
		{
			LPNMHDR pn = (LPNMHDR)lParam;
			switch (pn->code) {		
			case TCN_SELCHANGE:
				switch (pn->idFrom) {
				case IDC_TAB:
					int i = TabCtrl_GetCurFocus(m_hTab);
					switch (i) {
					case 0:
						ShowWindow(hTab0dialog, SW_SHOW);
						SetFocus(hTab0dialog);
						return 0;
					case 1:
						ShowWindow(hTab1dialog, SW_SHOW);
						SetFocus(hTab1dialog);
						return 0;
					case 2:
						ShowWindow(hTab2dialog, SW_SHOW);
						SetFocus(hTab2dialog);
						return 0;
					case 3:
						ShowWindow(hTab3dialog, SW_SHOW);
						SetFocus(hTab3dialog);
						return 0;
					case 4:
						ShowWindow(hTab4dialog, SW_SHOW);
						SetFocus(hTab4dialog);
						return 0;
					case 5:
						ShowWindow(hTab5dialog, SW_SHOW);
						SetFocus(hTab5dialog);
						return 0;
					case 6:
						ShowWindow(hTab6dialog, SW_SHOW);
						SetFocus(hTab6dialog);
						return 0;
					}
					return 0;
				}
				return 0;
			case TCN_SELCHANGING:
				switch (pn->idFrom) {
				case IDC_TAB:
					int i = TabCtrl_GetCurFocus(m_hTab);
					switch (i) {
					case 0:
						ShowWindow(hTab0dialog, SW_HIDE);
						break;
					case 1:
						ShowWindow(hTab1dialog, SW_HIDE);
						break;
					case 2:
						ShowWindow(hTab2dialog, SW_HIDE);
						break;
					case 3:
						ShowWindow(hTab3dialog, SW_HIDE);
						break;
					case 4:
						ShowWindow(hTab4dialog, SW_HIDE);
						break;
					case 5:
						ShowWindow(hTab5dialog, SW_HIDE);
						break;
					case 6:
						ShowWindow(hTab6dialog, SW_HIDE);
						//ShowWindow(hTab7dialog, SW_HIDE);
						break;
					}
					return 0;
				}
				return 0;
			}
			return 0;
		}
			
	}
    return 0;
}

