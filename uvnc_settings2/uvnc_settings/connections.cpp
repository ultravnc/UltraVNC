#include "stdafx.h"
#include "resource.h"

extern LONG QueryTimeout;
extern LONG QueryIfNoLogon;
extern char authhosts[150];
extern LONG QuerySetting;
extern LONG ConnectPriority;
extern LONG QueryAccept;
extern LONG LockSettings;
extern LONG IdleTimeout;
extern LONG IdleInputTimeout;
extern char accept_reject_mesg[512];
extern LONG keepAliveInterval;

bool initdone4=false;
BOOL CALLBACK DlgProcCON(HWND hwnd, UINT uMsg,
											   WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			initdone4=false;
			BOOL queryEnabled = (QuerySetting == 4);
			SendMessage(GetDlgItem(hwnd, IDQUERY), BM_SETCHECK, queryEnabled, 0);
			EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DRefuseOnly), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_QNOLOGON), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_AUTHHOST), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_QMESG), queryEnabled);

			SendMessage(GetDlgItem(hwnd, IDC_QNOLOGON), BM_SETCHECK, QueryIfNoLogon, 0);
			SetDlgItemInt(hwnd,IDQUERYTIMEOUT, QueryTimeout, FALSE);
			SetDlgItemInt(hwnd,IDC_IDLETIME, IdleTimeout, FALSE);
			SetDlgItemInt(hwnd,IDC_IDLETIMEINPUT, IdleInputTimeout, FALSE);
			SetDlgItemInt(hwnd,IDC_KINTERVAL, keepAliveInterval, FALSE);
			SetDlgItemText(hwnd,IDC_AUTHHOST,authhosts);
			SetDlgItemText(hwnd,IDC_QMESG,accept_reject_mesg);

			HWND hQuerySetting;
			switch (QueryAccept) {
			case 0:
				hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
				break;
			case 1:
				hQuerySetting = GetDlgItem(hwnd, IDC_DACCEPT);
				break;
			case 2:
				hQuerySetting = GetDlgItem(hwnd, IDC_DRefuseOnly);
				break;
			default:
				hQuerySetting = GetDlgItem(hwnd, IDC_DREFUSE);
			};
			SendMessage(hQuerySetting,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hLockSetting;
			switch (LockSettings) {
			case 1:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK);
				break;
			case 2:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF);
				break;
			default:
				hLockSetting = GetDlgItem(hwnd, IDC_LOCKSETTING_NOTHING);
			};
			SendMessage(hLockSetting,
				BM_SETCHECK,
				TRUE,
				0);

			HWND hmvSetting;
			switch (ConnectPriority) {
			case 0:
				hmvSetting = GetDlgItem(hwnd, IDC_MV1);
				break;
			case 1:
				hmvSetting = GetDlgItem(hwnd, IDC_MV2);
				break;
			case 2:
				hmvSetting = GetDlgItem(hwnd, IDC_MV3);
				break;
			case 3:
				hmvSetting = GetDlgItem(hwnd, IDC_MV4);
				break;
			};
			SendMessage(hmvSetting,
				BM_SETCHECK,
				TRUE,
				0);

			
			initdone4=true;
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{	
		case IDQUERY:
			{
			bool status=SendDlgItemMessage(hwnd, IDQUERY, BM_GETCHECK, 0, 0);
			if (status) QuerySetting=4;
			else QuerySetting=2;
			BOOL queryEnabled = (QuerySetting == 4);
			SendMessage(GetDlgItem(hwnd, IDQUERY), BM_SETCHECK, queryEnabled, 0);
			EnableWindow(GetDlgItem(hwnd, IDQUERYTIMEOUT), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DREFUSE), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DRefuseOnly), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_DACCEPT), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_QNOLOGON), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_AUTHHOST), queryEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_QMESG), queryEnabled);
			}
			break;

		case IDC_HELP2:
			if (lParam==3)
			
{
			char link[256];
			strcpy(link,"http://www.uvnc.com/webhelp/");
			strcat(link,"connections");
			strcat(link,".html");
			ShellExecute(GetDesktopWindow(), "open", link, "", 0, SW_SHOWNORMAL);
			}
			break;
		case IDOK:	
			{
			BOOL ok1;
			QueryTimeout=GetDlgItemInt(hwnd, IDQUERYTIMEOUT, &ok1, TRUE);
			QueryIfNoLogon=SendDlgItemMessage(hwnd, IDC_QNOLOGON, BM_GETCHECK, 0, 0);
			GetDlgItemText(hwnd, IDC_AUTHHOST, authhosts, 150);
			GetDlgItemText(hwnd, IDC_QMESG, accept_reject_mesg, 512);

			if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOCK), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					LockSettings=1;
				} else if (SendMessage(GetDlgItem(hwnd, IDC_LOCKSETTING_LOGOFF), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					LockSettings=2;
				} else {
					LockSettings=0;
				}

				if (SendMessage(GetDlgItem(hwnd, IDC_DREFUSE), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					QueryAccept=0;
				} else if (SendMessage(GetDlgItem(hwnd, IDC_DACCEPT), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					QueryAccept=1;
				} 
				else if (SendMessage(GetDlgItem(hwnd, IDC_DRefuseOnly), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					QueryAccept=2;
				} 

				if (SendMessage(GetDlgItem(hwnd, IDC_MV1), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					ConnectPriority=0;
				} else if (SendMessage(GetDlgItem(hwnd, IDC_MV2), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					ConnectPriority=1;
				} 
				 else if (SendMessage(GetDlgItem(hwnd, IDC_MV3), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					ConnectPriority=2;
				} else if (SendMessage(GetDlgItem(hwnd, IDC_MV4), BM_GETCHECK, 0, 0)
					== BST_CHECKED) {
					ConnectPriority=3;
				} 
			IdleTimeout=GetDlgItemInt(hwnd, IDC_IDLETIME, &ok1, TRUE);
			IdleInputTimeout=GetDlgItemInt(hwnd, IDC_IDLETIMEINPUT, &ok1, TRUE);
			keepAliveInterval=GetDlgItemInt(hwnd, IDC_KINTERVAL, &ok1, TRUE);

			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		}
		return 0;	
	}

	return 0;
}