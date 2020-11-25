#include "stdafx.h"
#include "resource.h"

extern LONG Primary;
extern LONG Secondary;
extern LONG TurboMode;
extern LONG PollUnderCursor;
extern LONG PollForeground;
extern LONG PollFullScreen;
extern LONG PollConsoleOnly;
extern LONG PollOnEventOnly;
extern LONG Driver;
extern bool ddEngine;
extern LONG Hook;
extern LONG RemoveWallpaper;
extern LONG RemoveAero;
bool CheckVideoDriver(bool Box);
extern LONG MaxCpu;
extern LONG MaxFps;

bool initdone5=false;
BOOL CALLBACK DlgProcCAP(HWND hwnd, UINT uMsg,
											   WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			initdone5=false;
			SendMessage(GetDlgItem(hwnd, IDC_TURBOMODE), BM_SETCHECK, TurboMode, 0);
			SendMessage(GetDlgItem(hwnd, IDC_DRIVER),BM_SETCHECK,Driver,0);
			SendMessage(GetDlgItem(hwnd, IDC_HOOK),BM_SETCHECK,Hook,0);
			SendMessage(GetDlgItem(hwnd, IDC_POLL_FULLSCREEN),BM_SETCHECK,PollFullScreen,0);
			SendMessage(GetDlgItem(hwnd, IDC_POLL_FOREGROUND),BM_SETCHECK,PollForeground,0);
			SendMessage(GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR),BM_SETCHECK,PollUnderCursor,0);
			SendMessage(GetDlgItem(hwnd, IDC_CONSOLE_ONLY),BM_SETCHECK,PollConsoleOnly,0);
			SendMessage(GetDlgItem(hwnd, IDC_REMOVE_WALLPAPER),BM_SETCHECK,RemoveWallpaper,0);
			SendMessage(GetDlgItem(hwnd, IDC_REMOVE_Aero),BM_SETCHECK,RemoveAero,0);
			SendMessage(GetDlgItem(hwnd, IDC_ONEVENT_ONLY),BM_SETCHECK,PollOnEventOnly,0);

            if (ddEngine) {
                ShowWindow(GetDlgItem(hwnd, IDC_CHECKDRIVER), false);
                //ShowWindow(GetDlgItem(hwnd, IDC_STATICELEVATED), false);
                SetWindowText(GetDlgItem(hwnd, IDC_DRIVER), "Desktop Duplication (restart on change required)");
                RECT rect;
                GetWindowRect(GetDlgItem(hwnd, IDC_DRIVER), &rect);
                POINT pt;
                pt.x = rect.left;
                pt.y = rect.top;
                ScreenToClient(hwnd, &pt);
                MoveWindow(GetDlgItem(hwnd, IDC_DRIVER), pt.x, pt.y, 410, 20, FALSE);
            }
			
			EnableWindow(GetDlgItem(hwnd, IDC_CONSOLE_ONLY),PollUnderCursor ||PollForeground);
			EnableWindow(GetDlgItem(hwnd, IDC_ONEVENT_ONLY),PollUnderCursor ||PollForeground);

			SendMessage(GetDlgItem(hwnd, IDC_PRIM),BM_SETCHECK,Primary,0);
			SendMessage(GetDlgItem(hwnd, IDC_SEC),BM_SETCHECK,Secondary,0);
			SetDlgItemInt(hwnd, IDC_MAXCPU, MaxCpu, false);
			SetDlgItemInt(hwnd, IDC_MAXFPS, MaxFps, false);
			initdone5=true;
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_HELP2:
			if (lParam==4)
			{
			char link[256];
			strcpy(link,"http://www.uvnc.com/webhelp/");
			strcat(link,"screencapture");
			strcat(link,".html");
			ShellExecute(GetDesktopWindow(), "open", link, "", 0, SW_SHOWNORMAL);
			}
			break;
		case IDOK:	
			{
				TurboMode= (LONG)SendDlgItemMessage(hwnd, IDC_TURBOMODE, BM_GETCHECK, 0, 0);
				PollUnderCursor= (LONG)SendDlgItemMessage(hwnd, IDC_POLL_UNDER_CURSOR, BM_GETCHECK, 0, 0);
				PollForeground= (LONG)SendDlgItemMessage(hwnd, IDC_POLL_FOREGROUND, BM_GETCHECK, 0, 0);
				PollFullScreen= (LONG)SendDlgItemMessage(hwnd, IDC_POLL_FULLSCREEN, BM_GETCHECK, 0, 0);
				PollConsoleOnly= (LONG)SendDlgItemMessage(hwnd, IDC_CONSOLE_ONLY, BM_GETCHECK, 0, 0);
				PollOnEventOnly= (LONG)SendDlgItemMessage(hwnd, IDC_ONEVENT_ONLY, BM_GETCHECK, 0, 0);
				Driver= (LONG)SendDlgItemMessage(hwnd, IDC_DRIVER, BM_GETCHECK, 0, 0);
				Primary= (LONG)SendDlgItemMessage(hwnd, IDC_PRIM, BM_GETCHECK, 0, 0);
				Secondary= (LONG)SendDlgItemMessage(hwnd, IDC_SEC, BM_GETCHECK, 0, 0);
				Hook= (LONG)SendDlgItemMessage(hwnd, IDC_HOOK, BM_GETCHECK, 0, 0);
				RemoveWallpaper= (LONG)SendDlgItemMessage(hwnd, IDC_REMOVE_WALLPAPER, BM_GETCHECK, 0, 0);
				RemoveAero= (LONG)SendDlgItemMessage(hwnd, IDC_REMOVE_Aero, BM_GETCHECK, 0, 0);
				MaxCpu = GetDlgItemInt(hwnd, IDC_MAXCPU, NULL, FALSE);
				MaxFps = GetDlgItemInt(hwnd, IDC_MAXFPS, NULL, FALSE);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDC_POLL_FOREGROUND:
		case IDC_POLL_UNDER_CURSOR:
			// User has clicked on one of the polling mode buttons
			// affected by the pollconsole and pollonevent options
			{
				// Get the poll-mode buttons
				HWND hPollForeground = GetDlgItem(hwnd, IDC_POLL_FOREGROUND);
				HWND hPollUnderCursor = GetDlgItem(hwnd, IDC_POLL_UNDER_CURSOR);

				// Determine whether to enable the modifier options
				BOOL enabled = (SendMessage(hPollForeground, BM_GETCHECK, 0, 0) == BST_CHECKED) ||
					(SendMessage(hPollUnderCursor, BM_GETCHECK, 0, 0) == BST_CHECKED);

				HWND hPollConsoleOnly = GetDlgItem(hwnd, IDC_CONSOLE_ONLY);
				EnableWindow(hPollConsoleOnly, enabled);

				HWND hPollOnEventOnly = GetDlgItem(hwnd, IDC_ONEVENT_ONLY);
				EnableWindow(hPollOnEventOnly, enabled);
			}
			return TRUE;
		case IDC_CHECKDRIVER:
			{
				CheckVideoDriver(1);
			}
			return TRUE;
	}
		return 0;	
	}

	return 0;
}