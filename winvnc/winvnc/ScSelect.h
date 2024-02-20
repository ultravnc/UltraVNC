/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


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
#endif

