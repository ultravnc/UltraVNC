/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2006 NANASI(KANTAN PROJECT). All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://kp774.com/

#define WINVER 0x0400
#define _WIN32_IE 0x0400
#include "stdhdrs.h"
#include "LinkLabel.h"
#include "res\resource.h"

#define ID_LINK_TIMER	1000

LRESULT CALLBACK LinkLabelProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

ATOM RegisterLinkLabel(HINSTANCE hInst)
{
	WNDCLASSEX wcex;

	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.hInstance = hInst;
	wcex.lpfnWndProc = LinkLabelProc;
	wcex.lpszClassName = CN_LINKLABEL;
	wcex.cbWndExtra = 8;

	return(RegisterClassEx(&wcex));
}

LRESULT CALLBACK LinkLabelProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HINSTANCE hInst;
	static COLORREF ttcol, tcol;
	char *cap, *jump;
	char temp[256];
	PAINTSTRUCT ps;
	HDC hDC;
	HCURSOR hCursor;
	HFONT hFont, hOldFont;
	POINT cp;
	RECT rc;
	int i;
	LOGFONT lf;

	switch(uMsg){
	case WM_CREATE:
		hInst = ((LPCREATESTRUCT)lParam)->hInstance;
		cap = (char *)GlobalAlloc(GPTR, 256);
		jump = (char *)GlobalAlloc(GPTR, 256);
		SetWindowLong(hWnd, 0, (LONG)cap);
		SetWindowLong(hWnd, 4, (LONG)jump);
		GetWindowText(hWnd, temp, 256);
		for(i = 0; temp[i] != 0; i++){
			if(temp[i] =='|'){
				lstrcpyn(cap, temp, i+1);
				cap[i] = 0;
				lstrcpy(jump, &temp[i+1]);
				break;
			}
		}
		SetTimer(hWnd, ID_LINK_TIMER, 50, NULL);
		break;
	case WM_SETCURSOR:
		hCursor = (HCURSOR)LoadImage(hInst, MAKEINTRESOURCE(IDC_FINGER), IMAGE_CURSOR, 0, 0, 0);
		SetCursor(hCursor);
		DeleteObject(hCursor);
		break;
	case WM_LBUTTONDOWN:
		cap = (char *)GetWindowLong(hWnd, 0);
		jump = (char *)GetWindowLong(hWnd, 4);
		if(ShellExecute(NULL, "open", jump, NULL, NULL, SW_SHOWNORMAL) <= (HINSTANCE)32)
			MessageBox(hWnd, "Could not launch a browser.", "UltraVNC", MB_ICONWARNING);
		break;
	case WM_PAINT:
		cap = (char *)GetWindowLong(hWnd, 0);
		jump = (char *)GetWindowLong(hWnd, 4);
		hDC = BeginPaint(hWnd, &ps);
		SetTextColor(hDC, tcol);
		SetBkMode(hDC, TRANSPARENT);
//		hFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "‚l‚r ‚oƒSƒVƒbƒN");
//		hFont = (HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0);
		GetObject((HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0), sizeof(LOGFONT), &lf);
		lf.lfUnderline = TRUE;
		hFont = CreateFontIndirect(&lf);
		hOldFont = (HFONT)SelectObject(hDC, hFont);
		TextOut(hDC, 0, 0, cap, lstrlen(cap));
		SelectObject(hDC, hOldFont);
		EndPaint(hWnd, &ps);
		DeleteObject(hFont);
		break;
	case WM_TIMER:
		GetCursorPos(&cp);
		GetWindowRect(hWnd, &rc);
		if(PtInRect(&rc, cp)){
			tcol = RGB(255, 0, 0);
		}
		else{
			tcol = RGB(0, 0, 255);
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_DESTROY:
		KillTimer(hWnd, ID_LINK_TIMER);
		cap = (char *)GetWindowLong(hWnd, 0);
		jump = (char *)GetWindowLong(hWnd, 4);
		GlobalFree((HANDLE)cap);
		GlobalFree((HANDLE)jump);
		break;
	default:
		return (DefWindowProc(hWnd, uMsg, wParam, lParam));
	}
	return 0;
}