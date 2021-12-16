/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////
#include "stdhdrs.h"

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <winsock2.h>
#include <windows.h>
#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#include <time.h>
HWND hwnd;
HINSTANCE hInst;
#ifndef LWA_COLORKEY
# define LWA_COLORKEY 1
#endif
#ifndef LWA_ALPHA 
# define LWA_ALPHA 2
#endif
#ifndef WS_EX_LAYERED
# define WS_EX_LAYERED 0x80000
#endif
#include "stdhdrs.h"
#include "resource.h"
#include "vncservice.h"


int wd=0;
int ht=0;

HBITMAP DoGetBkGndBitmap2(IN CONST UINT uBmpResId)
{
    static HBITMAP hbmBkGnd = NULL;
    if (NULL == hbmBkGnd)
    {
		char WORKDIR[MAX_PATH];
		char mycommand[MAX_PATH];
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH)){
			char* p = strrchr(WORKDIR, '\\');
			if (p == NULL) return 0;
			*p = '\0';
		}
		strcpy_s(mycommand,WORKDIR);
		strcat_s(mycommand,"\\background.bmp");

		hbmBkGnd = (HBITMAP)LoadImage(NULL, mycommand, IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE);
		if (hbmBkGnd ==NULL){
			hbmBkGnd = (HBITMAP)LoadImage(
            GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO64),IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		}
		BITMAPINFOHEADER h2;
		h2.biSize=sizeof(h2);
		h2.biBitCount=0;
		HDC hxdc=CreateDC("DISPLAY",NULL,NULL,NULL);  
		GetDIBits(hxdc, hbmBkGnd, 0, 0, NULL, (BITMAPINFO*)&h2, DIB_RGB_COLORS);
		wd=h2.biWidth; ht=h2.biHeight;
		DeleteDC(hxdc);
        if (NULL == hbmBkGnd)
            hbmBkGnd = (HBITMAP)-1;
    }
    return (hbmBkGnd == (HBITMAP)-1)
        ? NULL : hbmBkGnd;
}


BOOL DoSDKEraseBkGnd2( IN CONST HDC hDC,IN CONST COLORREF crBkGndFill )
{
    HBITMAP hbmBkGnd = DoGetBkGndBitmap2(0);
    if (hDC && hbmBkGnd)
    {
        RECT rc;
        if ((ERROR != GetClipBox(hDC, &rc)) && !IsRectEmpty(&rc)){
            HDC hdcMem = CreateCompatibleDC(hDC);
            if (hdcMem){
                HBRUSH hbrBkGnd = CreateSolidBrush(crBkGndFill);
                if (hbrBkGnd){
                    HGDIOBJ hbrOld = SelectObject(hDC, hbrBkGnd);
                    if (hbrOld){
                        SIZE size = {(rc.right-rc.left), (rc.bottom-rc.top)};
                        if (PatBlt(hDC, rc.left, rc.top, size.cx, size.cy, PATCOPY)){
                            HGDIOBJ hbmOld = SelectObject(hdcMem, hbmBkGnd);
                            if (hbmOld){
								StretchBlt(hDC,0,0,size.cx,size.cy,hdcMem,0,0,wd,ht,SRCCOPY);
                                SelectObject(hdcMem, hbmOld);
                            }
                        }
                        SelectObject(hDC, hbrOld);
                    }
                    DeleteObject(hbrBkGnd);
                }
                DeleteDC(hdcMem);
            }
        }
    }
    return TRUE;
}


static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    switch (uMsg){
		case WM_CREATE:
			SetTimer(hwnd,100,20,NULL);
			break;
		case WM_TIMER:
			if (wParam==100) 
				SetWindowPos(hwnd,HWND_TOPMOST,0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
							    
		case WM_ERASEBKGND:
			DoSDKEraseBkGnd2((HDC)wParam, RGB(0,0,0));
			return true;
		case WM_CTLCOLORSTATIC:
			SetBkMode((HDC) wParam, TRANSPARENT);
			return (LONG_PTR) GetStockObject(NULL_BRUSH);
		case WM_DESTROY:
			KillTimer(hwnd,100);
			PostQuitMessage (0);
			break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

bool create_black_window(void)
{
	WNDCLASSEX wndClass;
	ZeroMemory (&wndClass, sizeof (wndClass));
    wndClass.cbSize        = sizeof (wndClass);
    wndClass.style         = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = WndProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = 0;
    wndClass.hInstance     = hInst;
    wndClass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
    wndClass.hIconSm       = NULL;
    wndClass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
    wndClass.lpszMenuName  = NULL;
    wndClass.lpszClassName = "blackscreen";

        if (!RegisterClassEx(&wndClass)) {
 //               return false;
        }

        RECT clientRect;
        clientRect.left = 0;
        clientRect.top = 0;
        clientRect.right = GetSystemMetrics(SM_CXSCREEN);
        clientRect.bottom = GetSystemMetrics(SM_CYSCREEN);

        UINT x(GetSystemMetrics(SM_XVIRTUALSCREEN));
        UINT y(GetSystemMetrics(SM_YVIRTUALSCREEN));
        UINT cx(GetSystemMetrics(SM_CXVIRTUALSCREEN));
        UINT cy(GetSystemMetrics(SM_CYVIRTUALSCREEN));

        clientRect.left = x;
        clientRect.top = y;
		clientRect.right = x +  cx;
		clientRect.bottom = y +  cy;

        AdjustWindowRect (&clientRect, WS_CAPTION, FALSE);
        hwnd = CreateWindowEx (WS_EX_TOOLWINDOW, "blackscreen", "blackscreen",
                               WS_POPUP  | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER ,
                               CW_USEDEFAULT,CW_USEDEFAULT,cx,cy,NULL,NULL, hInst,NULL);
		typedef DWORD (WINAPI *PSLWA)(HWND, DWORD, BYTE, DWORD);

	PSLWA pSetLayeredWindowAttributes=NULL;
	HMODULE hDLL = LoadLibrary ("user32");
	if (hDLL) pSetLayeredWindowAttributes = (PSLWA) GetProcAddress(hDLL,"SetLayeredWindowAttributes");

#ifndef _X64
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	style = GetWindowLong(hwnd, GWL_STYLE);
	style &= ~(WS_DLGFRAME | WS_THICKFRAME);
	SetWindowLong(hwnd, GWL_STYLE, style);
#else
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style &= ~(WS_DLGFRAME | WS_THICKFRAME);
	SetWindowLongPtr(hwnd, GWL_STYLE, style);
#endif

	if (pSetLayeredWindowAttributes != NULL) {
#ifndef _X64
		SetWindowLong (hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) |WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST);
#else
		SetWindowLongPtr (hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) |WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOPMOST);
#endif
	    ShowWindow (hwnd, SW_SHOWNORMAL);
	}
	if (pSetLayeredWindowAttributes != NULL)
		pSetLayeredWindowAttributes (hwnd, RGB(255,255,255), 255, LWA_ALPHA);
	SetWindowPos(hwnd,HWND_TOPMOST,x,y,cx,cy, SWP_FRAMECHANGED|SWP_NOACTIVATE);
	SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
	return true;
}

DWORD WINAPI BlackWindow(LPVOID lpParam)
{
	HDESK desktop;
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy{};

	char new_name[256]{};
	if (desktop){
		GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy);
		SetThreadDesktop(desktop);
	}

	create_black_window();
	MSG msg;
	while (GetMessage(&msg,0,0,0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	vnclog.Print(LL_INTERR, VNCLOG("end BlackWindow \n"));
	SetThreadDesktop(old_desktop);
	if (desktop) CloseDesktop(desktop);

	return 0;
}

RECT g_rect;

static LRESULT CALLBACK WndBorderProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (uMsg) {
	case WM_PAINT: {
		HGDIOBJ original = NULL;
		hdc = BeginPaint(hwnd, &ps);
		original = SelectObject(hdc, GetStockObject(DC_PEN));		
		HPEN hPen = CreatePen(PS_DASHDOTDOT, 5, RGB(255, 0, 0));
		SelectObject(hdc, hPen);
		Rectangle(hdc, g_rect.left, g_rect.top, g_rect.right, g_rect.bottom);
		SelectObject(hdc, original);
		DeleteObject(hPen);
		EndPaint(hwnd, &ps);
	}
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

bool create_border_window(RECT rect)
{
	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(wndClass));
	wndClass.cbSize = sizeof(wndClass);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndBorderProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInst;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hIconSm = NULL;
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = "borderscreen";

	RegisterClassEx(&wndClass);
	hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, "borderscreen", "borderscreen",
		WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN ,
		CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInst, NULL);
	typedef DWORD(WINAPI* PSLWA)(HWND, DWORD, BYTE, DWORD);

	PSLWA pSetLayeredWindowAttributes = NULL;
	HMODULE hDLL = LoadLibrary("user32");
	if (hDLL) pSetLayeredWindowAttributes = (PSLWA)GetProcAddress(hDLL, "SetLayeredWindowAttributes");

#ifndef _X64
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	style = GetWindowLong(hwnd, GWL_STYLE);
	style &= ~(WS_DLGFRAME | WS_THICKFRAME);
	SetWindowLong(hwnd, GWL_STYLE, style);
#else
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style = GetWindowLongPtr(hwnd, GWL_STYLE);
	style &= ~(WS_DLGFRAME | WS_THICKFRAME);
	SetWindowLongPtr(hwnd, GWL_STYLE, style);
#endif

	if (pSetLayeredWindowAttributes != NULL) {
#ifndef _X64
		SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#else
		SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#endif
		ShowWindow(hwnd, SW_SHOWNORMAL);
	}
	if (pSetLayeredWindowAttributes != NULL)
		pSetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
	SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
	SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
	return true;
}

DWORD WINAPI BorderWindow(LPVOID lpParam)
{
	Sleep(1000);
	HDESK desktop;
	desktop = OpenInputDesktop(0, FALSE,
		DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
		DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
		DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
		DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
	);

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy{};

	char new_name[256]{};
	if (desktop) {
		GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy);
		SetThreadDesktop(desktop);
	}

	create_border_window(g_rect);
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	SetThreadDesktop(old_desktop);
	if (desktop) CloseDesktop(desktop);

	return 0;
}
