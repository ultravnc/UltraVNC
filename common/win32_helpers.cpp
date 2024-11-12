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
#include <winsock2.h>
#include <windows.h>
#include "win32_helpers.h"
#ifdef _VIEWER
#include "../vncviewer/res/resource.h"
#else
#include "../winvnc/winvnc/resource.h"
#endif

namespace helper {

void SafeSetMsgResult(HWND hwnd, LPARAM result)
{
#ifndef _X64
	SetWindowLong(hwnd, DWL_MSGRESULT, result);
#else
	SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result);
#endif
}

void SafeSetWindowUserData(HWND hwnd, LPARAM lParam)
{
#ifndef _X64
	SetWindowLong(hwnd, GWL_USERDATA, lParam);
#else
	SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
#endif
}

HINSTANCE SafeGetWindowInstance(HWND hWnd)
{
#ifndef _X64
    HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hWnd,GWL_HINSTANCE);
#else
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hWnd,GWLP_HINSTANCE);
#endif
    return hInstance;
}

LONG SafeGetWindowProc(HWND hWnd)
{
#ifndef _X64
    LONG pWndProc = GetWindowLong(hWnd, GWL_WNDPROC);
#else
    //TODO: get rid of LONG_PTR return warning for x64
    LONG pWndProc = GetWindowLongPtr(hWnd, GWLP_WNDPROC);
#endif
    return pWndProc;
}

void SafeSetWindowProc(HWND hWnd, LONG_PTR pWndProc)
{
#ifndef _X64
    SetWindowLong(hWnd, GWL_WNDPROC, pWndProc);
#else
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, pWndProc);
#endif
}

void close_handle(HANDLE& h)
{
    if (h != INVALID_HANDLE_VALUE) 
    {
        ::CloseHandle(h);
        h = INVALID_HANDLE_VALUE;
    }
}

DynamicFnBase::DynamicFnBase(const TCHAR* dllName, const char* fnName) : fnPtr(0), dllHandle(0) {
  dllHandle = LoadLibrary(dllName);
  if (!dllHandle) {
    return;
  }
  fnPtr = (void *) GetProcAddress(dllHandle, fnName);
}

DynamicFnBase::~DynamicFnBase() {
  if (dllHandle)
    FreeLibrary(dllHandle);
}

bool yesnoUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, char* szHeader, char* body, char* okStr, char* cancelStr, char* checkbox, BOOL& bCheckboxChecked)
{
    wchar_t w_header[128];
    wchar_t w_body[1024];
    wchar_t w_checkbox[1024];
    wchar_t w_okStr[512];
    wchar_t w_cancelStr[512];
    size_t outSize;
    mbstowcs_s(&outSize, w_header, szHeader, strlen(szHeader) + 1);
    mbstowcs_s(&outSize, w_body, body, strlen(body) + 1);
    if (strlen(checkbox) > 0)
        mbstowcs_s(&outSize, w_checkbox, checkbox, strlen(checkbox) + 1);
    mbstowcs_s(&outSize, w_okStr, okStr, strlen(okStr) + 1);
    mbstowcs_s(&outSize, w_cancelStr, cancelStr, strlen(cancelStr) + 1);

    HRESULT hr;
    TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
    int nClickedBtn;
#ifdef _VIEWER
    LPCWSTR szTitle = L"UltraVNC Viewer";
#else
    LPCWSTR szTitle = L"UltraVNC Server";
#endif
    TASKDIALOG_BUTTON aCustomButtons[] = {
        { 1000, w_okStr},
        { 1001, w_cancelStr}
    };
    tdc.cbSize = sizeof(tdc);
    tdc.hInstance = hInst;
    tdc.hwndParent = m_hWnd;
    tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
    tdc.pButtons = aCustomButtons;
    tdc.cButtons = _countof(aCustomButtons);
    tdc.pszWindowTitle = szTitle;
    tdc.nDefaultButton = 1001;
#ifdef _VIEWER
    tdc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);// TD_INFORMATION_ICON;
#else
    tdc.pszMainIcon = MAKEINTRESOURCEW(IDI_WINVNC);// TD_INFORMATION_ICON;
#endif
    tdc.pszMainInstruction = w_header;
    tdc.pszContent = w_body;
    if (strlen(checkbox) > 0)
        tdc.pszVerificationText = w_checkbox;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, &bCheckboxChecked);

    if (SUCCEEDED(hr) && 1000 == nClickedBtn)
        return true;
    return false;
}

void yesUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, char* body, char* szHeader, int icon)
{
    wchar_t w_header[128];
    wchar_t w_body[1024];
    size_t outSize;

    mbstowcs_s(&outSize, w_header, szHeader, strlen(szHeader) + 1);
    mbstowcs_s(&outSize, w_body, body, strlen(body) + 1);

    HRESULT hr;
    TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
    int nClickedBtn;
#ifdef _VIEWER
    LPCWSTR szTitle = L"UltraVNC Viewer";
#else
    LPCWSTR szTitle = L"UltraVNC Server";
#endif

    tdc.cbSize = sizeof(tdc);
    tdc.hInstance = hInst;
    tdc.hwndParent = m_hWnd;
    tdc.dwCommonButtons = TDCBF_YES_BUTTON;
    tdc.pszWindowTitle = szTitle;
    tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;

    switch (icon) {
    case MB_ICONEXCLAMATION:
        tdc.pszMainIcon = TD_WARNING_ICON;
        break;
    case MB_ICONINFORMATION:
        tdc.pszMainIcon = TD_INFORMATION_ICON;
        break;
    case MB_ICONERROR:
        tdc.pszMainIcon = TD_ERROR_ICON;
        break;
    default:
#ifdef _VIEWER
        tdc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);// TD_INFORMATION_ICON;
#else
        tdc.pszMainIcon = MAKEINTRESOURCEW(IDI_WINVNC);// TD_INFORMATION_ICON;
#endif
        break;
    }
    tdc.pszMainInstruction = w_header;
    tdc.pszContent = w_body;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
}

} // namespace helper
