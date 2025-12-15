// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#pragma once
#ifdef SC_20
	#include "stdhdrs.h"
	#include "CommCtrl.h"

namespace ScSelect
{
	extern wchar_t	Balloon1Title[150];
	extern wchar_t	Balloon2Title[150];
	extern wchar_t	Balloon1A[150];
	extern wchar_t	Balloon1B[150];
	extern wchar_t	Balloon1C[150];
	extern wchar_t	Balloon2A[150];
	extern wchar_t	Balloon2B[150];
	extern wchar_t	Balloon2C[150];
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
#endif // SC_20
