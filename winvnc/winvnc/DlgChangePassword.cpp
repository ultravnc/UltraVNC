#include "DlgChangePassword.h"
#include "resource.h"
#include "common/win32_helpers.h"
#include "SettingsManager.h"

extern HINSTANCE	hInstResDLL;

INT_PTR CALLBACK dialogProcChangePassword(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DlgChangePassword* _this;
    if (uMsg == WM_INITDIALOG) {
        _this = (DlgChangePassword*)lParam;
        helper::SafeSetWindowUserData(hwnd, lParam);
    }
    else
        _this = (DlgChangePassword*)helper::SafeGetWindowUserData<DlgChangePassword>(hwnd);

    switch (uMsg) {
    case WM_INITDIALOG:
		return _this->InitDialog(hwnd);
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDCANCEL:
				EndDialog(hwnd, FALSE);
				return TRUE;
			case IDOK:
				_this->onOK(hwnd);
				return TRUE;
			default:
				return TRUE;
		}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
    }
    return 0;
}
DlgChangePassword::DlgChangePassword()
{
}
DlgChangePassword::~DlgChangePassword() {}

int DlgChangePassword::ShowDlg(HWND parentWindow, char* title, int passwordSize)
{
	strcpy(this->title, title);
	this->passwordSize = passwordSize;

	int a = DialogBoxParam(
		hInstResDLL, MAKEINTRESOURCE(IDD_CHANGE_PASSWORD),
		parentWindow, dialogProcChangePassword,
		(LPARAM)this );
	return a;
}

bool DlgChangePassword::InitDialog(HWND hwnd)
{
	DlgChangePasswordHwnd = hwnd;
	SetForegroundWindow(hwnd);
	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	SetWindowText(hwnd, title);
	SendMessage(GetDlgItem(DlgChangePasswordHwnd, IDC_PASSWORD_1), EM_SETLIMITTEXT, passwordSize, 0);
	SendMessage(GetDlgItem(DlgChangePasswordHwnd, IDC_PASSWORD_2), EM_SETLIMITTEXT, passwordSize, 0);
	SetDlgItemText(hwnd, IDC_PASSWORD_1, "~~~~~~~~");
	SetDlgItemText(hwnd, IDC_PASSWORD_2, "~~~~~~~~");
	return true;
}

void DlgChangePassword::onOK(HWND hwnd)
{
	char passwd2[1024];
	memset(passwd, '\0', 1024); 
	memset(passwd2, '\0', 1024);
	GetDlgItemText(hwnd, IDC_PASSWORD_1, (LPSTR)&passwd, 1024);
	GetDlgItemText(hwnd, IDC_PASSWORD_2, (LPSTR)&passwd2, 1024);
	if ((strlen(passwd) == 0 || strcmp(passwd, "~~~~~~~~") == 0) && settings->getAuthRequired() != false) {
		helper::yesUVNCMessageBox(hInstResDLL, hwnd, "Password can not be empty.", "UltraVNC warning", MB_ICONWARNING);
		SetFocus(GetDlgItem(hwnd, IDC_PASSWORD_1));
		return;
	}
	if ((strcmp(passwd, passwd2) != 0 || strcmp(passwd, "~~~~~~~~") == 0)) {
		helper::yesUVNCMessageBox(hInstResDLL, hwnd, "The verification passwords is not the same.", "UltraVNC warning", MB_ICONWARNING);
		SetFocus(GetDlgItem(hwnd, IDC_PASSWORD_2));
		return;
	}
	if (strcmp(passwd, "~~~~~~~~") == 0)
		strcpy(passwd, "");
	EndDialog(hwnd, TRUE);
}

char* DlgChangePassword::getPassword()
{
	return passwd;
}
