#define HOTKEY_CAD 0
#define HOTKEY_FULL 1
#define HOTKEY_REFRESH 2
#define HOTKEY_SHOWCON 3
#define HOTKEY_CE 4
#define HOTKEY_CUS 5
#define HOTKEY_STATUS 6
#define HOTKEY_CLOSE 7
#define HOTKEY_HIDE 8
#define HOTKEY_BLANK 9
#define HOTKEY_FILE 10
#define HOTKEY_SINGLE 11
#define HOTKEY_FULLDESK 12
#define HOTKEY_CHAT 13

#ifndef __HOTKEYDLG_H
#define __HOTKEYDLG_H

class HotkeyDlg
{
private:
	HWND m_parent;
	HWND m_hwnd;
	HINSTANCE m_hinst;
	LPCTSTR m_ini;

public:
	HotkeyDlg(HWND hwnd, HINSTANCE hinst, LPCTSTR ini);
	~HotkeyDlg();
	
	BOOL CALLBACK m_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void Go();
	void SetupModCombo(int item, char* name);
	void SetupKeyCombo(int item, char* name);
	void Save(int mod, int key, char* name, int id);
};

#endif