#include <windows.h>
#include <stdio.h>
#include "res\\resource.h"
#include "HotkeyDlg.h"

BOOL CALLBACK SkeletonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HotkeyDlg::HotkeyDlg(HWND hwnd, HINSTANCE hinst, LPCTSTR ini)
{
	m_parent = hwnd;
	m_hinst = hinst;
	m_ini = ini;
}

HotkeyDlg::~HotkeyDlg()
{
	RemoveProp(m_hwnd, "HotkeyDlg");
}

void HotkeyDlg::Go()
{
	m_hwnd = CreateDialog(m_hinst, MAKEINTRESOURCE(IDD_HOTKEYS), m_parent, (DLGPROC)SkeletonProc);
	SetProp(m_hwnd, "HotkeyDlg", (HANDLE)this);
	SetupModCombo(IDC_MOD_CAD, "Cad");
	SetupKeyCombo(IDC_KEY_CAD, "Cad");
	SetupModCombo(IDC_MOD_FULL, "Full");
	SetupKeyCombo(IDC_KEY_FULL, "Full");
	SetupModCombo(IDC_MOD_REFRESH, "Refresh");
	SetupKeyCombo(IDC_KEY_REFRESH, "Refresh");
	SetupModCombo(IDC_MOD_SHOWCON, "Showcon");
	SetupKeyCombo(IDC_KEY_SHOWCON, "Showcon");
	SetupModCombo(IDC_MOD_CE, "Ce");
	SetupKeyCombo(IDC_KEY_CE, "Ce");
	SetupModCombo(IDC_MOD_CUS, "Cus");
	SetupKeyCombo(IDC_KEY_CUS, "Cus");
	SetupModCombo(IDC_MOD_STATUS, "Status");
	SetupKeyCombo(IDC_KEY_STATUS, "Status");
	SetupModCombo(IDC_MOD_CLOSE, "Close");
	SetupKeyCombo(IDC_KEY_CLOSE, "Clode");
	SetupModCombo(IDC_MOD_HIDE, "Hide");
	SetupKeyCombo(IDC_KEY_HIDE, "Hide");
	SetupModCombo(IDC_MOD_BLANK, "Blank");
	SetupKeyCombo(IDC_KEY_BLANK, "Blank");
	SetupModCombo(IDC_MOD_FILE, "File");
	SetupKeyCombo(IDC_KEY_FILE, "File");
	SetupModCombo(IDC_MOD_SINGLE, "Single");
	SetupKeyCombo(IDC_KEY_SINGLE, "Single");
	SetupModCombo(IDC_MOD_FULLDESK, "Fulldesk");
	SetupKeyCombo(IDC_KEY_FULLDESK, "Fulldesk");
	SetupModCombo(IDC_MOD_CHAT, "Chat");
	SetupKeyCombo(IDC_KEY_CHAT, "Chat");
}

void HotkeyDlg::SetupModCombo(int item, char* name)
{
	HWND hwnd = GetDlgItem(m_hwnd, item);
	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"N/A");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Win");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Ctrl");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Ctrl+Alt");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Ctrl+Shift");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Shift+Alt");

	// Select the correct one
	char i[3];
	GetPrivateProfileString("Hotkeys", name, "00", i, 3, m_ini);
	SendMessage(hwnd, CB_SETCURSEL, (WPARAM)i[0]-'0', 0);
}

void HotkeyDlg::SetupKeyCombo(int item, char* name)
{
	HWND hwnd = GetDlgItem(m_hwnd, item);
	SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"N/A");	

	for (int c=0; c<26; c++) // A-Z
	{
		char temp[2] = "";
		sprintf(temp, "%c\0", 'A'+c);
		SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)temp);
	}
#ifdef VC2005
	for (int c=0; c<10; c++) // 0-9
	{
		char temp[2] = "";
		sprintf(temp, "%d\0", c);
		SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)temp);
	}
	for (int c=1; c<13; c++) // F1-F12
	{
		char temp[4] = "";
		sprintf(temp, "F%d\0", c);
		SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)temp);
	}
#else
	for (c=0; c<10; c++) // 0-9
	{
		char temp[2] = "";
		sprintf(temp, "%d\0", c);
		SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)temp);
	}
	for (c=1; c<13; c++) // F1-F12
	{
		char temp[4] = "";
		sprintf(temp, "F%d\0", c);
		SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)temp);
	}
#endif	
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Right");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Left");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Up");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Down");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Home");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"End");
	SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)"Ins");

	// Select the correct one
	char i[4];
	GetPrivateProfileString("Hotkeys", name, "000", i, 4, m_ini);
	SendMessage(hwnd, CB_SETCURSEL, (WPARAM)atoi(i+1), 0);
}

void HotkeyDlg::Save(int mod, int key, char* name, int id)
{
	HWND hwnd_mod = GetDlgItem(m_hwnd, mod);
	HWND hwnd_key = GetDlgItem(m_hwnd, key);
	char i[4]="";
	int mi=0, ki=0;
	mi=SendMessage(hwnd_mod, CB_GETCURSEL, 0, 0);
	ki=SendMessage(hwnd_key, CB_GETCURSEL, 0, 0);
	
	UnregisterHotKey(m_parent, id);
	if (mi > 0 && ki > 0)
	{
		UINT mod;
		UINT key;

		if (mi == 1) mod = MOD_WIN;
		else if (mi == 2) mod = MOD_CONTROL;
		else if (mi == 3) mod = MOD_CONTROL | MOD_ALT;
		else if (mi == 4) mod = MOD_CONTROL | MOD_SHIFT;
		else if (mi == 5) mod = MOD_SHIFT | MOD_ALT;

		if (ki <= 26) key = 'A' + ki - 1;	
		else if (ki <= 36) key = '0' + ki - 27;
		else if (ki <= 48) key = VK_F1 + ki - 37;
		else if (ki == 49) key = VK_RIGHT;
		else if (ki == 50) key = VK_LEFT;
		else if (ki == 51) key = VK_UP;
		else if (ki == 52) key = VK_DOWN;
		else if (ki == 53) key = VK_HOME;
		else if (ki == 54) key = VK_END;
		else if (ki == 55) key = VK_INSERT;

		if (!RegisterHotKey(m_parent, id, mod, key))
		{
			char errMsg[256] = "";
			sprintf(errMsg, "Unable to register hotkey for %s.", name);
			MessageBox(m_hwnd, errMsg, "Hotkey Registration Error", MB_SYSTEMMODAL | MB_OK | MB_ICONERROR);
		}
	}
	
	sprintf(i, "%d%d\0", mi, ki);
	WritePrivateProfileString("Hotkeys", name, i, m_ini);
}

BOOL CALLBACK SkeletonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HotkeyDlg* dlg = (HotkeyDlg*)GetProp(hwnd, "HotkeyDlg");
	return dlg->m_proc(hwnd, msg, wParam, lParam);
}

BOOL CALLBACK HotkeyDlg::m_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					switch (LOWORD(wParam))
					{
						case IDOK:
						{
							Save(IDC_MOD_CAD, IDC_KEY_CAD, "Cad", HOTKEY_CAD);
							Save(IDC_MOD_FULL, IDC_KEY_FULL, "Full", HOTKEY_FULL);
							Save(IDC_MOD_REFRESH, IDC_KEY_REFRESH, "Refresh", HOTKEY_REFRESH);
							Save(IDC_MOD_SHOWCON, IDC_KEY_SHOWCON, "Showcon", HOTKEY_SHOWCON);
							Save(IDC_MOD_CE, IDC_KEY_CE, "Ce", HOTKEY_CE);
							Save(IDC_MOD_CUS,IDC_KEY_CUS, "Cus", HOTKEY_CUS);
							Save(IDC_MOD_STATUS,IDC_KEY_STATUS, "Status", HOTKEY_STATUS);
							Save(IDC_MOD_CLOSE, IDC_KEY_CLOSE, "Clode", HOTKEY_CLOSE);
							Save(IDC_MOD_HIDE, IDC_KEY_HIDE, "Hide", HOTKEY_HIDE);
							Save(IDC_MOD_BLANK, IDC_KEY_BLANK, "Blank", HOTKEY_BLANK);
							Save(IDC_MOD_FILE,IDC_KEY_FILE, "File", HOTKEY_FILE);
							Save(IDC_MOD_SINGLE,IDC_KEY_SINGLE, "Single", HOTKEY_SINGLE);
							Save(IDC_MOD_FULLDESK, IDC_KEY_FULLDESK, "Fulldesk", HOTKEY_FULLDESK);
							Save(IDC_MOD_CHAT, IDC_KEY_CHAT, "Chat", HOTKEY_CHAT);
						}
						case IDCANCEL:
						{
							EndDialog(hwnd, 0);
						}
						break;
					}
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}