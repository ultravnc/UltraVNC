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


// Hyperlinks.cpp
//
// Copyright 2002 Neal Stublen
// All rights reserved.
//
// http://www.awesoftware.com
//

#include <windows.h>

#include "Hyperlinks.h"

#ifndef _X64 
#define PROP_ORIGINAL_FONT		TEXT("_Hyperlink_Original_Font_")
#define PROP_ORIGINAL_PROC		TEXT("_Hyperlink_Original_Proc_")
#define PROP_STATIC_HYPERLINK	TEXT("_Hyperlink_From_Static_")
#define PROP_UNDERLINE_FONT		TEXT("_Hyperlink_Underline_Font_")


LRESULT CALLBACK _HyperlinkParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnOrigProc = (WNDPROC) GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC) wParam;
			HWND hwndCtl = (HWND) lParam;

			BOOL fHyperlink = (NULL != GetProp(hwndCtl, PROP_STATIC_HYPERLINK));
			if (fHyperlink)
			{
				LRESULT lr = CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
				SetTextColor(hdc, RGB(0, 0, 192));
				return lr;
			}

			break;
		}
	case WM_DESTROY:
		{
			SetWindowLong(hwnd, GWL_WNDPROC, (LONG) pfnOrigProc);
			RemoveProp(hwnd, PROP_ORIGINAL_PROC);
			break;
		}
	}
	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK _HyperlinkProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnOrigProc = (WNDPROC) GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_DESTROY:
		{
			SetWindowLong(hwnd, GWL_WNDPROC, (LONG) pfnOrigProc);
			RemoveProp(hwnd, PROP_ORIGINAL_PROC);

			HFONT hOrigFont = (HFONT) GetProp(hwnd, PROP_ORIGINAL_FONT);
			SendMessage(hwnd, WM_SETFONT, (WPARAM) hOrigFont, 0);
			RemoveProp(hwnd, PROP_ORIGINAL_FONT);

			HFONT hFont = (HFONT) GetProp(hwnd, PROP_UNDERLINE_FONT);
			DeleteObject(hFont);
			RemoveProp(hwnd, PROP_UNDERLINE_FONT);

			RemoveProp(hwnd, PROP_STATIC_HYPERLINK);

			break;
		}
	case WM_MOUSEMOVE:
		{
			if (GetCapture() != hwnd)
			{
				HFONT hFont = (HFONT) GetProp(hwnd, PROP_UNDERLINE_FONT);
				SendMessage(hwnd, WM_SETFONT, (WPARAM) hFont, FALSE);
				InvalidateRect(hwnd, NULL, FALSE);
				SetCapture(hwnd);
			}
			else
			{
				RECT rect;
				GetWindowRect(hwnd, &rect);

				POINT pt = { LOWORD(lParam), HIWORD(lParam) };
				ClientToScreen(hwnd, &pt);

				if (!PtInRect(&rect, pt))
				{
					HFONT hFont = (HFONT) GetProp(hwnd, PROP_ORIGINAL_FONT);
					SendMessage(hwnd, WM_SETFONT, (WPARAM) hFont, FALSE);
					InvalidateRect(hwnd, NULL, FALSE);
					ReleaseCapture();
				}
			}
			break;
		}
	case WM_SETCURSOR:
		{
			// Since IDC_HAND is not available on all operating systems,
			// we will load the arrow cursor if IDC_HAND is not present.
			HCURSOR hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND));
			if (NULL == hCursor)
			{
				hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
			}
			SetCursor(hCursor);
			return TRUE;
		}
	}

	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}

BOOL ConvertStaticToHyperlink(HWND hwndCtl)
{
	// Subclass the parent so we can color the controls as we desire.

	HWND hwndParent = GetParent(hwndCtl);
	if (NULL != hwndParent)
	{
		WNDPROC pfnOrigProc = (WNDPROC) GetWindowLong(hwndParent, GWL_WNDPROC);
		if (pfnOrigProc != _HyperlinkParentProc)
		{
			SetProp(hwndParent, PROP_ORIGINAL_PROC, (HANDLE) pfnOrigProc);
			SetWindowLong(hwndParent, GWL_WNDPROC, (LONG) (WNDPROC) _HyperlinkParentProc);
		}
	}

	// Make sure the control will send notifications.

	DWORD dwStyle = GetWindowLong(hwndCtl, GWL_STYLE);
	SetWindowLong(hwndCtl, GWL_STYLE, dwStyle | SS_NOTIFY);

	// Subclass the existing control.

	WNDPROC pfnOrigProc = (WNDPROC) GetWindowLong(hwndCtl, GWL_WNDPROC);
	SetProp(hwndCtl, PROP_ORIGINAL_PROC, (HANDLE) pfnOrigProc);
	SetWindowLong(hwndCtl, GWL_WNDPROC, (LONG) (WNDPROC) _HyperlinkProc);

	// Create an updated font by adding an underline.

	HFONT hOrigFont = (HFONT) SendMessage(hwndCtl, WM_GETFONT, 0, 0);
	SetProp(hwndCtl, PROP_ORIGINAL_FONT, (HANDLE) hOrigFont);

	LOGFONT lf;
	GetObject(hOrigFont, sizeof(lf), &lf);
	lf.lfUnderline = TRUE;

	HFONT hFont = CreateFontIndirect(&lf);
	SetProp(hwndCtl, PROP_UNDERLINE_FONT, (HANDLE) hFont);

	// Set a flag on the control so we know what color it should be.

	SetProp(hwndCtl, PROP_STATIC_HYPERLINK, (HANDLE) 1);

	return TRUE;
}

#else

#define PROP_ORIGINAL_FONT		TEXT("_Hyperlink_Original_Font_")
#define PROP_ORIGINAL_PROC		TEXT("_Hyperlink_Original_Proc_")
#define PROP_STATIC_HYPERLINK	TEXT("_Hyperlink_From_Static_")
#define PROP_UNDERLINE_FONT		TEXT("_Hyperlink_Underline_Font_")


LRESULT CALLBACK _HyperlinkParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnOrigProc = (WNDPROC)GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
	{
		HDC hdc = (HDC)wParam;
		HWND hwndCtl = (HWND)lParam;

		BOOL fHyperlink = (NULL != GetProp(hwndCtl, PROP_STATIC_HYPERLINK));
		if (fHyperlink)
		{
			LRESULT lr = CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
			SetTextColor(hdc, RGB(0, 0, 192));
			return lr;
		}

		break;
	}
	case WM_DESTROY:
	{
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pfnOrigProc));
		RemoveProp(hwnd, PROP_ORIGINAL_PROC);
		break;
	}
	}
	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}

LRESULT CALLBACK _HyperlinkProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pfnOrigProc = (WNDPROC)GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_DESTROY:
	{
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(pfnOrigProc));
		RemoveProp(hwnd, PROP_ORIGINAL_PROC);

		HFONT hOrigFont = (HFONT)GetProp(hwnd, PROP_ORIGINAL_FONT);
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hOrigFont, 0);
		RemoveProp(hwnd, PROP_ORIGINAL_FONT);

		HFONT hFont = (HFONT)GetProp(hwnd, PROP_UNDERLINE_FONT);
		DeleteObject(hFont);
		RemoveProp(hwnd, PROP_UNDERLINE_FONT);

		RemoveProp(hwnd, PROP_STATIC_HYPERLINK);

		break;
	}
	case WM_MOUSEMOVE:
	{
		if (GetCapture() != hwnd)
		{
			HFONT hFont = (HFONT)GetProp(hwnd, PROP_UNDERLINE_FONT);
			SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, FALSE);
			InvalidateRect(hwnd, NULL, FALSE);
			SetCapture(hwnd);
		}
		else
		{
			RECT rect;
			GetWindowRect(hwnd, &rect);

			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			ClientToScreen(hwnd, &pt);

			if (!PtInRect(&rect, pt))
			{
				HFONT hFont = (HFONT)GetProp(hwnd, PROP_ORIGINAL_FONT);
				SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, FALSE);
				InvalidateRect(hwnd, NULL, FALSE);
				ReleaseCapture();
			}
		}
		break;
	}
	case WM_SETCURSOR:
	{
		// Since IDC_HAND is not available on all operating systems,
		// we will load the arrow cursor if IDC_HAND is not present.
		HCURSOR hCursor = LoadCursor(NULL, IDC_HAND);
		if (NULL == hCursor)
		{
			hCursor = LoadCursor(NULL, IDC_ARROW);
		}
		SetCursor(hCursor);
		return TRUE;
	}
	}

	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}


BOOL ConvertStaticToHyperlink(HWND hwndCtl)
{
	// Subclass the parent so we can color the controls as we desire.

	HWND hwndParent = GetParent(hwndCtl);
	if (NULL != hwndParent)
	{
		WNDPROC pfnOrigProc = (WNDPROC)GetWindowLongPtr(hwndParent, GWLP_WNDPROC);
		if (pfnOrigProc != _HyperlinkParentProc)
		{
			SetProp(hwndParent, PROP_ORIGINAL_PROC, (HANDLE)pfnOrigProc);
			SetWindowLongPtr(hwndParent, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_HyperlinkParentProc));
		}
	}

	// Make sure the control will send notifications.

	LONG_PTR dwStyle = GetWindowLongPtr(hwndCtl, GWL_STYLE);
	SetWindowLongPtr(hwndCtl, GWL_STYLE, dwStyle | SS_NOTIFY);

	// Subclass the existing control.

	WNDPROC pfnOrigProc = (WNDPROC)GetWindowLongPtr(hwndCtl, GWLP_WNDPROC);
	SetProp(hwndCtl, PROP_ORIGINAL_PROC, (HANDLE)pfnOrigProc);
	SetWindowLongPtr(hwndCtl, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(_HyperlinkProc));

	// Create an updated font by adding an underline.

	HFONT hOrigFont = (HFONT)SendMessage(hwndCtl, WM_GETFONT, 0, 0);
	SetProp(hwndCtl, PROP_ORIGINAL_FONT, (HANDLE)hOrigFont);

	LOGFONT lf;
	GetObject(hOrigFont, sizeof(lf), &lf);
	lf.lfUnderline = TRUE;

	HFONT hFont = CreateFontIndirect(&lf);
	SetProp(hwndCtl, PROP_UNDERLINE_FONT, (HANDLE)hFont);

	// Set a flag on the control so we know what color it should be.

	SetProp(hwndCtl, PROP_STATIC_HYPERLINK, (HANDLE)1);

	return TRUE;
}
#endif

BOOL ConvertStaticToHyperlink(HWND hwndParent, UINT uiCtlId)
{
	return ConvertStaticToHyperlink(GetDlgItem(hwndParent, uiCtlId));
}
