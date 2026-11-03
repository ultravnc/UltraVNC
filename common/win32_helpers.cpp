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
#include <stdio.h>
#include <vector>
#include <mbstring.h>
#include <ctype.h>
#include <stdlib.h>
#include "win32_helpers.h"
#include "rfb.h"
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

// Helper to log a merge failure with the failing path and error.
static void MoveDirLog(LPCWSTR msg, LPCWSTR path)
{
    DWORD dwErr = GetLastError();
    WCHAR buf[MAX_PATH * 4 + 128];
    _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"[MoveDirContentsInto] %s: %s (err=%lu)\n", msg, path, dwErr);
    OutputDebugStringW(buf);
}

// Recursive worker on already-prefixed paths.
static bool MoveDirContentsIntoImpl(LPCWSTR srcDirL, LPCWSTR dstDirL, unsigned depth)
{
    if (depth > 64)
        return false;

    DWORD dstAttr = GetFileAttributesW(dstDirL);
    if (dstAttr == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateDirectoryW(dstDirL, NULL))
        {
            MoveDirLog(L"CreateDirectoryW failed", dstDirL);
            return false;
        }
    }
    else if (!(dstAttr & FILE_ATTRIBUTE_DIRECTORY))
    {
        MoveDirLog(L"destination is not a directory", dstDirL);
        return false; // can't merge a directory into a file
    }

    std::vector<WCHAR> pattern(MAX_PATH * 4);
    _snwprintf_s(pattern.data(), pattern.size(), _TRUNCATE, L"%s\\*", srcDirL);
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(pattern.data(), &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        MoveDirLog(L"FindFirstFileW failed", srcDirL);
        return false;
    }

    bool ok = true;
    do {
        if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L".."))
            continue;

        std::vector<WCHAR> srcChild(MAX_PATH * 4), dstChild(MAX_PATH * 4);
        _snwprintf_s(srcChild.data(), srcChild.size(), _TRUNCATE, L"%s\\%s", srcDirL, fd.cFileName);
        _snwprintf_s(dstChild.data(), dstChild.size(), _TRUNCATE, L"%s\\%s", dstDirL, fd.cFileName);

        DWORD a = GetFileAttributesW(dstChild.data());
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY))
            {
                MoveDirLog(L"source dir conflicts with destination file", dstChild.data());
                ok = false; // file/dir name conflict
                continue;
            }
            // Always create/ensure the target dir then merge recursively.
            // This is slower than a whole-subtree rename, but it works across
            // volumes and with long paths, and the source subdir is removed
            // by the recursive call once it is empty.
            if (a == INVALID_FILE_ATTRIBUTES && !CreateDirectoryW(dstChild.data(), NULL))
            {
                MoveDirLog(L"CreateDirectoryW failed for subdir", dstChild.data());
                ok = false;
                continue;
            }
            if (!MoveDirContentsIntoImpl(srcChild.data(), dstChild.data(), depth + 1))
                ok = false;
        }
        else
        {
            if (a != INVALID_FILE_ATTRIBUTES)
            {
                if (a & FILE_ATTRIBUTE_DIRECTORY)
                {
                    MoveDirLog(L"source file conflicts with destination dir", dstChild.data());
                    ok = false; // dir/file name conflict
                    continue;
                }
                if (a & FILE_ATTRIBUTE_READONLY)
                    SetFileAttributesW(dstChild.data(), a & ~FILE_ATTRIBUTE_READONLY);
            }
            if (!MoveFileExW(srcChild.data(), dstChild.data(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
            {
                MoveDirLog(L"MoveFileExW failed", srcChild.data());
                ok = false;
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (ok)
    {
        if (!RemoveDirectoryW(srcDirL))
        {
            MoveDirLog(L"RemoveDirectoryW failed on source", srcDirL);
            ok = false;
        }
    }
    return ok;
}

bool MoveDirContentsInto(LPCWSTR srcDir, LPCWSTR dstDir)
{
    WCHAR srcL[MAX_PATH * 4], dstL[MAX_PATH * 4];
    PrefixLongPathW(srcDir, srcL, _countof(srcL));
    PrefixLongPathW(dstDir, dstL, _countof(dstL));

    WCHAR info[MAX_PATH * 4 + 64];
    _snwprintf_s(info, _countof(info), _TRUNCATE, L"[MoveDirContentsInto] %s -> %s\n", srcL, dstL);
    OutputDebugStringW(info);

    return MoveDirContentsIntoImpl(srcL, dstL, 0);
}

bool IsSafeFileNameW(LPCWSTR name)
{
    if (name == NULL || name[0] == L'\0')
        return false;
    if (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0)
        return false;
    size_t len = wcslen(name);
    if (name[len - 1] == L'.' || name[len - 1] == L' ')
        return false;
    for (const WCHAR* p = name; *p; p++)
    {
        switch (*p)
        {
        case L'\\': case L'/': case L':': case L'*': case L'?':
        case L'"': case L'<': case L'>': case L'|':
            return false;
        }
    }
    return true;
}

bool IsSafeRelativePathW(LPCWSTR name)
{
    if (name == NULL || name[0] == L'\0')
        return false;
    if (name[0] == L'\\' || name[0] == L'/')  // rooted or UNC
        return false;
    if (name[1] == L':')                      // drive absolute (C:...)
        return false;

    const WCHAR* p = name;
    while (*p)
    {
        const WCHAR* e = p;
        while (*e && *e != L'\\' && *e != L'/')
            e++;
        size_t len = e - p;
        if (len > 0)
        {
            if ((len == 1 && p[0] == L'.') || (len == 2 && p[0] == L'.' && p[1] == L'.'))
                return false;
            for (size_t i = 0; i < len; i++)
                if (p[i] == L':')
                    return false;
        }
        p = (*e) ? e + 1 : e;
    }
    return true;
}

bool IsDirectChildPathW(LPCWSTR parentDir, LPCWSTR path)
{
    if (parentDir == NULL || path == NULL)
        return false;

    WCHAR fullParent[MAX_PATH * 4];
    WCHAR fullChild[MAX_PATH * 4];
    if (!GetFullPathNameW(parentDir, MAX_PATH * 4, fullParent, NULL))
        return false;
    if (!GetFullPathNameW(path, MAX_PATH * 4, fullChild, NULL))
        return false;

    size_t parentLen = wcslen(fullParent);
    while (parentLen > 0 && fullParent[parentLen - 1] == L'\\')
        fullParent[--parentLen] = L'\0';
    size_t childLen = wcslen(fullChild);
    if (parentLen == 0 || childLen <= parentLen + 1)
        return false;
    if (_wcsnicmp(fullChild, fullParent, parentLen) != 0)
        return false;
    if (fullChild[parentLen] != L'\\')
        return false;
    // The remainder must be a single path component.
    if (wcschr(fullChild + parentLen + 1, L'\\') != NULL)
        return false;
    return true;
}

bool IsExpectedDirZipReply(const char* szReceived, const char* szRequested, bool fUnicodeWireNames, bool fOldProtocol)
{
    if (szReceived == NULL || szRequested == NULL)
        return false;

    // The requested path must end in a directory marker: "[ name ]" for the
    // current protocol, "(name)" for the <= RC18 protocol (the request is
    // rewritten for old servers). _mbsrchr handles DBCS names like strrchr
    // is meant to in the file-transfer code.
    const char* reqBase = (const char*)_mbsrchr((const unsigned char*)szRequested, '\\');
    reqBase = reqBase ? reqBase + 1 : szRequested;
    size_t reqLen = strlen(reqBase);
    bool bBracketMarker = (reqLen >= 5 &&
        reqBase[0] == rfbDirPrefix[0] && reqBase[1] == rfbDirPrefix[1] &&
        reqBase[reqLen - 2] == rfbDirSuffix[0] && reqBase[reqLen - 1] == rfbDirSuffix[1]);
    bool bParenMarker = (fOldProtocol && reqLen >= 3 &&
        reqBase[0] == '(' && reqBase[reqLen - 1] == ')');
    if (!bBracketMarker && !bParenMarker)
        return false;

    // The reply basename must be !UVNCDIR-<name>.zip
    const char* rcvBase = (const char*)_mbsrchr((const unsigned char*)szReceived, '\\');
    rcvBase = rcvBase ? rcvBase + 1 : szReceived;
    if (strncmp(rcvBase, rfbZipDirectoryPrefix, strlen(rfbZipDirectoryPrefix)) != 0)
        return false;
    const char* mid = rcvBase + strlen(rfbZipDirectoryPrefix);
    size_t midLen = strlen(mid);
    if (midLen <= 4 || _stricmp(mid + midLen - 4, ".zip") != 0)
        return false;
    midLen -= 4;
    if (midLen == 0 || midLen >= MAX_PATH * 3)
        return false;

    if (fUnicodeWireNames)
    {
        // Hex-encoded UTF-8 dirname; must decode to a prefix of the requested
        // directory name (senders truncate the hex at 200 characters).
        // Unicode-capable servers always use the "[ name ]" request marker.
        if (!bBracketMarker)
            return false;
        if ((midLen % 2) != 0 || midLen > 200)
            return false;
        const char* reqName = reqBase + 2;
        size_t reqNameLen = reqLen - 4;
        if (midLen / 2 > reqNameLen)
            return false;
        for (size_t i = 0; i < midLen / 2; i++)
        {
            if (!isxdigit((unsigned char)mid[i * 2]) || !isxdigit((unsigned char)mid[i * 2 + 1]))
                return false;
            char hexByte[3] = { mid[i * 2], mid[i * 2 + 1], '\0' };
            if ((unsigned char)strtol(hexByte, NULL, 16) != (unsigned char)reqName[i])
                return false;
        }
        return true;
    }

    // Old servers reply with a plain (8.3) directory name. It becomes the
    // local folder name, so only accept a safe, traversal-free name.
    char szMid[MAX_PATH * 3];
    memcpy(szMid, mid, midLen);
    szMid[midLen] = '\0';
    WCHAR szMidW[MAX_PATH * 4];
    MultiByteToWideChar(CP_ACP, 0, szMid, -1, szMidW, MAX_PATH * 4);
    return IsSafeFileNameW(szMidW);
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
