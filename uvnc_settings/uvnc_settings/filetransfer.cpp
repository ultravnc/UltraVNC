#include "stdafx.h"
#include "resource.h"

extern LONG EnableRemoteInputs;
extern LONG DisableLocalInputs;//
extern LONG EnableJapInput;//
extern LONG FileTransferEnabled;//
extern LONG FTUserImpersonation;//
extern LONG BlankMonitorEnabled;//
extern LONG BlankInputsOnly; //PGM
extern LONG BlackAlphaBlending;
extern LONG FTTimeout;//

bool initdone3=false;
BOOL CALLBACK DlgProcOFT(HWND hwnd, UINT uMsg,
											   WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case WM_INITDIALOG: 
		{	
			initdone3=false;
			SendMessage(GetDlgItem(hwnd, IDC_FILETRANSFER), BM_SETCHECK, FileTransferEnabled, 0);
			SendMessage(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), BM_SETCHECK, FTUserImpersonation, 0);
			SendMessage(GetDlgItem(hwnd, IDC_BLANK), BM_SETCHECK, BlankMonitorEnabled, 0);
			SendMessage(GetDlgItem(hwnd, IDC_BLANK2), BM_SETCHECK, BlankInputsOnly, 0); //PGM
			SendMessage(GetDlgItem(hwnd, IDC_JAP_INPUTS), BM_SETCHECK, EnableJapInput, 0);

			SendMessage(GetDlgItem(hwnd, IDC_DISABLE_LOCAL_INPUTS), BM_SETCHECK, DisableLocalInputs, 0);
			SendMessage(GetDlgItem(hwnd, IDC_JAP_INPUTS), BM_SETCHECK, EnableJapInput, 0);
			SendMessage(GetDlgItem(hwnd, IDC_DISABLE_INPUTS), BM_SETCHECK, !EnableRemoteInputs, 0);

			SetDlgItemInt(hwnd, IDC_FTTIMEOUT, FTTimeout, FALSE);

			CheckDlgButton(hwnd, IDC_POWER,(!BlackAlphaBlending) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwnd, IDC_ALPHA,(BlackAlphaBlending) ? BST_CHECKED : BST_UNCHECKED);

			EnableWindow(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), FileTransferEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_FTTIMEOUT), FileTransferEnabled);

			EnableWindow(GetDlgItem(hwnd, IDC_POWER), BlankMonitorEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_ALPHA), BlankMonitorEnabled);
			
			initdone3=true;
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{	
		case IDOK:

			EnableRemoteInputs=!SendDlgItemMessage(hwnd, IDC_DISABLE_INPUTS, BM_GETCHECK, 0, 0);
			DisableLocalInputs=SendDlgItemMessage(hwnd, IDC_DISABLE_LOCAL_INPUTS, BM_GETCHECK, 0, 0);
			EnableJapInput=SendDlgItemMessage(hwnd, IDC_JAP_INPUTS, BM_GETCHECK, 0, 0);
			FileTransferEnabled=SendDlgItemMessage(hwnd, IDC_FILETRANSFER, BM_GETCHECK, 0, 0);
			FTUserImpersonation=SendDlgItemMessage(hwnd, IDC_FTUSERIMPERSONATION_CHECK, BM_GETCHECK, 0, 0);
			BlankMonitorEnabled=SendDlgItemMessage(hwnd, IDC_BLANK, BM_GETCHECK, 0, 0);
			BlankInputsOnly=SendDlgItemMessage(hwnd, IDC_BLANK2, BM_GETCHECK, 0, 0); //PGM
			BlackAlphaBlending=SendDlgItemMessage(hwnd, IDC_ALPHA, BM_GETCHECK, 0, 0);
			BOOL ok1;
			FTTimeout=GetDlgItemInt(hwnd, IDC_FTTIMEOUT, &ok1, TRUE);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDC_FILETRANSFER:
			FileTransferEnabled=SendDlgItemMessage(hwnd, IDC_FILETRANSFER, BM_GETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hwnd, IDC_FTUSERIMPERSONATION_CHECK), FileTransferEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_FTTIMEOUT), FileTransferEnabled);
			break;
		case IDC_BLANK:
			BlankMonitorEnabled=SendDlgItemMessage(hwnd, IDC_BLANK, BM_GETCHECK, 0, 0);
			EnableWindow(GetDlgItem(hwnd, IDC_POWER), BlankMonitorEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_ALPHA), BlankMonitorEnabled);
			EnableWindow(GetDlgItem(hwnd, IDC_BLANK2), BlankMonitorEnabled); //PGM
			if (!BlankMonitorEnabled) //PGM
				SendMessage(GetDlgItem(hwnd, IDC_BLANK2), BM_SETCHECK, BlankMonitorEnabled, 0); //PGM
			BlankInputsOnly=SendDlgItemMessage(hwnd, IDC_BLANK2, BM_GETCHECK, 0, 0); //PGM
			break;
		case IDC_BLANK2: //PGM
			BlankInputsOnly=SendDlgItemMessage(hwnd, IDC_BLANK2, BM_GETCHECK, 0, 0); //PGM
			break; //PGM
		}
		return 0;	
	}

	return 0;
}