// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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

// Add the \\?\ (or \\?\UNC\) prefix so paths longer than MAX_PATH work.
static void PrefixLongPathW(LPCWSTR in, WCHAR* out, int outLen)
{
    if (in[0] == L'\\' && in[1] == L'\\' && in[2] == L'?' && in[3] == L'\\')
        wcscpy_s(out, outLen, in); // already prefixed
    else if (in[0] == L'\\' && in[1] == L'\\')
        _snwprintf_s(out, outLen, _TRUNCATE, L"\\\\?\\UNC\\%s", in + 2);
    else if (in[0] && in[1] == L':')
        _snwprintf_s(out, outLen, _TRUNCATE, L"\\\\?\\%s", in);
    else
        wcscpy_s(out, outLen, in);
}

// Recursive worker on already-prefixed paths.
static bool MoveDirContentsIntoImpl(LPCWSTR srcDirL, LPCWSTR dstDirL)
{
    DWORD dstAttr = GetFileAttributesW(dstDirL);
    if (dstAttr == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(dstDirL, NULL))
            return false;
    }
    else if (!(dstAttr & FILE_ATTRIBUTE_DIRECTORY))
        return false; // can't merge a directory into a file

    WCHAR pattern[MAX_PATH * 4];
    _snwprintf_s(pattern, _countof(pattern), _TRUNCATE, L"%s\\*", srcDirL);
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return false;

    bool ok = true;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
            continue;

        WCHAR srcChild[MAX_PATH * 4], dstChild[MAX_PATH * 4];
        _snwprintf_s(srcChild, _countof(srcChild), _TRUNCATE, L"%s\\%s", srcDirL, fd.cFileName);
        _snwprintf_s(dstChild, _countof(dstChild), _TRUNCATE, L"%s\\%s", dstDirL, fd.cFileName);

        DWORD a = GetFileAttributesW(dstChild);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (a == INVALID_FILE_ATTRIBUTES)
            {
                // No conflict: move the whole subtree in one shot
                if (!MoveFileExW(srcChild, dstChild, MOVEFILE_COPY_ALLOWED))
                    ok = false;
            }
            else if (a & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (!MoveDirContentsIntoImpl(srcChild, dstChild))
                    ok = false;
            }
            else
                ok = false; // file/dir name conflict
        }
        else
        {
            if (a != INVALID_FILE_ATTRIBUTES)
            {
                if (a & FILE_ATTRIBUTE_DIRECTORY)
                {
                    ok = false; // dir/file name conflict
                    continue;
                }
                if (a & FILE_ATTRIBUTE_READONLY)
                    SetFileAttributesW(dstChild, a & ~FILE_ATTRIBUTE_READONLY);
            }
            if (!MoveFileExW(srcChild, dstChild, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
                ok = false;
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (ok)
        RemoveDirectoryW(srcDirL);
    return ok;
}

bool MoveDirContentsInto(LPCWSTR srcDir, LPCWSTR dstDir)
{
    WCHAR srcL[MAX_PATH * 4], dstL[MAX_PATH * 4];
    PrefixLongPathW(srcDir, srcL, _countof(srcL));
    PrefixLongPathW(dstDir, dstL, _countof(dstL));
    return MoveDirContentsIntoImpl(srcL, dstL);
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
    MultiByteToWideChar(CP_ACP, 0, szHeader, -1, w_header, 128);
    MultiByteToWideChar(CP_ACP, 0, body, -1, w_body, 1024);
    if (strlen(checkbox) > 0)
        MultiByteToWideChar(CP_ACP, 0, checkbox, -1, w_checkbox, 1024);
    MultiByteToWideChar(CP_ACP, 0, okStr, -1, w_okStr, 512);
    MultiByteToWideChar(CP_ACP, 0, cancelStr, -1, w_cancelStr, 512);

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

bool yesUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, char* body, char* szHeader, int icon)
{
    wchar_t w_header[128];
    wchar_t w_body[2048];
    MultiByteToWideChar(CP_ACP, 0, szHeader, -1, w_header, 128);
    MultiByteToWideChar(CP_ACP, 0, body, -1, w_body, 2048);

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
    tdc.dwCommonButtons = TDCBF_OK_BUTTON;
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
    if (SUCCEEDED(hr) && TDCBF_OK_BUTTON == nClickedBtn)
        return true;
    return false;
}

bool yesUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, wchar_t* body, wchar_t* szHeader, int icon)
{
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
    tdc.dwCommonButtons = TDCBF_OK_BUTTON;
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
        tdc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);
#else
        tdc.pszMainIcon = MAKEINTRESOURCEW(IDI_WINVNC);
#endif
        break;
    }
    tdc.pszMainInstruction = szHeader;
    tdc.pszContent = body;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
    if (SUCCEEDED(hr) && TDCBF_OK_BUTTON == nClickedBtn)
        return true;
    return false;
}

bool yesnoUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, wchar_t* szHeader, wchar_t* body, wchar_t* okStr, wchar_t* cancelStr, wchar_t* checkbox, BOOL& bCheckboxChecked)
{
    HRESULT hr;
    TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
    int nClickedBtn;
#ifdef _VIEWER
    LPCWSTR szTitle = L"UltraVNC Viewer";
#else
    LPCWSTR szTitle = L"UltraVNC Server";
#endif
    TASKDIALOG_BUTTON aCustomButtons[] = {
        { 1000, okStr },
        { 1001, cancelStr }
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
    tdc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);
#else
    tdc.pszMainIcon = MAKEINTRESOURCEW(IDI_WINVNC);
#endif
    tdc.pszMainInstruction = szHeader;
    tdc.pszContent = body;
    if (checkbox && wcslen(checkbox) > 0)
        tdc.pszVerificationText = checkbox;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, &bCheckboxChecked);

    if (SUCCEEDED(hr) && 1000 == nClickedBtn)
        return true;
    return false;
}

} // namespace helper
