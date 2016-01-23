#include "stdafx.h"
#include "resource.h"


extern LONG DisableTrayIcon;
extern LONG Rdpmode;
extern LONG DefaultScale;
extern LONG Avilog;
extern char path[512];
extern LONG DebugMode;
extern LONG DebugLevel;
extern LONG BUseRegistry;
extern LONG kickrdp;
extern char servicecmdline[256];


bool initdone6=false;
BOOL CALLBACK DlgProcMISC(HWND hwnd, UINT uMsg,
											   WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			initdone6=false;
			SendMessage( GetDlgItem(hwnd, IDC_DISABLETRAY), BM_SETCHECK, DisableTrayIcon, 0);
			SendMessage(GetDlgItem(hwnd, IDC_RDPMODE), BM_SETCHECK, Rdpmode, 0);
			SendMessage( GetDlgItem(hwnd, IDC_KICKRDP), BM_SETCHECK, kickrdp, 0);
			SendMessage( GetDlgItem(hwnd, IDC_DISINI), BM_SETCHECK, BUseRegistry, 0);
			SetDlgItemInt(hwnd, IDC_SCALE, DefaultScale, false);
			SetDlgItemText(hwnd, IDC_SCL, servicecmdline);
			bool debug=false;
			if (DebugMode >= 2)
			{
				debug=true;
			   CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
			}
		   else
			   CheckDlgButton(hwnd, IDC_LOG, BST_UNCHECKED);

			if (DebugMode==2)
			{
				SendMessage(GetDlgItem(hwnd, IDC_FILE),BM_SETCHECK,TRUE,0);
				SendMessage(GetDlgItem(hwnd, IDC_WINDOW),BM_SETCHECK,FALSE,0);
			}
			if (DebugMode==4)
			{
				SendMessage(GetDlgItem(hwnd, IDC_WINDOW),BM_SETCHECK,TRUE,0);
				SendMessage(GetDlgItem(hwnd, IDC_FILE),BM_SETCHECK,FALSE,0);
			}

			if (DebugLevel>=10)
			{
				SendMessage(GetDlgItem(hwnd, IDC_ALL),BM_SETCHECK,TRUE,0);
				SendMessage(GetDlgItem(hwnd, IDC_INFO),BM_SETCHECK,FALSE,0);
				SendMessage(GetDlgItem(hwnd, IDC_WARN),BM_SETCHECK,FALSE,0);

			}
			if (DebugLevel==9)
			{
				SendMessage(GetDlgItem(hwnd, IDC_ALL),BM_SETCHECK,FALSE,0);
				SendMessage(GetDlgItem(hwnd, IDC_INFO),BM_SETCHECK,TRUE,0);
				SendMessage(GetDlgItem(hwnd, IDC_WARN),BM_SETCHECK,FALSE,0);
			}
			if (DebugLevel<=8)
			{
				SendMessage(GetDlgItem(hwnd, IDC_ALL),BM_SETCHECK,FALSE,0);
				SendMessage(GetDlgItem(hwnd, IDC_INFO),BM_SETCHECK,FALSE,0);
				SendMessage(GetDlgItem(hwnd, IDC_WARN),BM_SETCHECK,TRUE,0);
			}
			EnableWindow(GetDlgItem(hwnd, IDC_WINDOW), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_FILE), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_ALL), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_INFO), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_WARN), debug);



			if (Avilog)
		   {
			   if (strlen(path)==0)
				{
					char WORKDIR[MAX_PATH];
				if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
					{
					char* p = strrchr(WORKDIR, '\\');
					if (p == NULL) return true;
					*p = '\0';
					}
					strcpy(path,WORKDIR);
				}

			   SetDlgItemText(hwnd, IDC_EDIT_PATH, path);
			   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
			   CheckDlgButton(hwnd, IDC_VIDEO, BST_CHECKED);
		   }
		   else
		   {
			   if (strlen(path)==0)
				{
					char WORKDIR[MAX_PATH];
				if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
					{
					char* p = strrchr(WORKDIR, '\\');
					if (p == NULL) return true;
					*p = '\0';
					}
					strcpy(path,WORKDIR);
				}
			   SetDlgItemText(hwnd, IDC_EDIT_PATH, path);
			   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
			   CheckDlgButton(hwnd, IDC_VIDEO, BST_UNCHECKED);
		   }

			initdone6=true;
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_HELP2:
			if (lParam==5)
			
{
			char link[256];
			strcpy(link,"http://www.uvnc.com/webhelp/");
			strcat(link,"misc");
			strcat(link,".html");
			ShellExecute(GetDesktopWindow(), "open", link, "", 0, SW_SHOWNORMAL);
			}
			break;
		case IDOK:	
			{
				DisableTrayIcon=IsDlgButtonChecked(hwnd, IDC_DISABLETRAY);
				Rdpmode = IsDlgButtonChecked(hwnd, IDC_RDPMODE);
				BUseRegistry=IsDlgButtonChecked(hwnd, IDC_DISINI);
				kickrdp=IsDlgButtonChecked(hwnd, IDC_KICKRDP);
				DefaultScale=GetDlgItemInt(hwnd, IDC_SCALE, NULL, FALSE);
				if (DefaultScale < 1 || DefaultScale > 9) DefaultScale = 1;
				Avilog=IsDlgButtonChecked(hwnd, IDC_VIDEO);
				GetDlgItemText(hwnd, IDC_EDIT_PATH, path,512);
				GetDlgItemText(hwnd, IDC_SCL, servicecmdline,256);
				bool log_checked=IsDlgButtonChecked(hwnd, IDC_LOG);
				bool window_checked=IsDlgButtonChecked(hwnd, IDC_WINDOW);
				bool file_checked=IsDlgButtonChecked(hwnd, IDC_FILE);
				bool debug=false;
				if (log_checked)
				{
					debug=true;
					if (window_checked) DebugMode=4;
					if (file_checked) DebugMode=2;
				}
				else
				{
					DebugMode=0;
				}
				bool all_checked=IsDlgButtonChecked(hwnd, IDC_ALL);
				bool info_checked=IsDlgButtonChecked(hwnd, IDC_INFO);
				bool warn_checked=IsDlgButtonChecked(hwnd, IDC_WARN);
				if (all_checked)DebugLevel=10;
				if (info_checked)DebugLevel=9;
				if (warn_checked)DebugLevel=8;
				
				
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case  IDC_LOG:
			{
				bool log_checked=IsDlgButtonChecked(hwnd, IDC_LOG);
				bool window_checked=IsDlgButtonChecked(hwnd, IDC_WINDOW);
				bool file_checked=IsDlgButtonChecked(hwnd, IDC_FILE);
				bool debug=false;
				if (log_checked)
				{
					debug=true;
					if (window_checked) DebugMode=4;
					if (file_checked) DebugMode=2;
				}
				else
				{
					DebugMode=0;
				}
				bool all_checked=IsDlgButtonChecked(hwnd, IDC_ALL);
				bool info_checked=IsDlgButtonChecked(hwnd, IDC_INFO);
				bool warn_checked=IsDlgButtonChecked(hwnd, IDC_WARN);
				if (all_checked)DebugLevel=10;
				if (info_checked)DebugLevel=9;
				if (warn_checked)DebugLevel=8;

				EnableWindow(GetDlgItem(hwnd, IDC_WINDOW), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_FILE), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_ALL), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_INFO), debug);
			EnableWindow(GetDlgItem(hwnd, IDC_WARN), debug);



			}
			break;
		case IDC_VIDEO:
			{
				if (IsDlgButtonChecked(hwnd, IDC_VIDEO))
				   {
					   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), true);
					   
				   }
				   else
				   {
					   EnableWindow(GetDlgItem(hwnd, IDC_EDIT_PATH), false);
					   
				   }
				break;
			}
		case IDC_CLEAR:
			{
				char WORKDIR[MAX_PATH];
				strcpy(WORKDIR,path);
				strcat(WORKDIR,"\\");
				strcat(WORKDIR,"codec.cfg");
				DeleteFile(WORKDIR);
				break;
			}
		
		}
		return 0;	
	}

	return 0;
}