#pragma once
#ifdef SC_20

#include "stdhdrs.h"
#include "CommCtrl.h"

namespace ScSelect
{
	extern char	Balloon1Title[150];
	extern char	Balloon2Title[150];
	extern char	Balloon1A[150];
	extern char	Balloon1B[150];
	extern char	Balloon1C[150];
	extern char	Balloon2A[150];
	extern char	Balloon2B[150];
	extern char	Balloon2C[150];
	extern LONG old_pref;
	extern bool g_dis_uac;
	extern bool	g_wallpaper_enabled;
	extern char	idStr[15];
	int toUnicode(wchar_t* buffer, const char* str);
	void Disbale_UAC_for_admin_run_elevated();
	void Disbale_UAC_for_admin();
	void Restore_UAC_for_admin_elevated();
	void Restore_UAC_for_admin(int i);
	HBITMAP DoGetBkGndBitmap(IN CONST UINT uBmpResId);
	HBITMAP DoGetBkGndBitmap3(IN CONST UINT uBmpResId);
	BOOL DoSDKEraseBkGnd(IN CONST HDC hDC, IN CONST COLORREF crBkGndFill);
	BOOL CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
	char *InitSC(HINSTANCE hInstance, PSTR szCmdLine);
	void ExpandBox(HWND hDlg, bool fExpand);
};
#endif

