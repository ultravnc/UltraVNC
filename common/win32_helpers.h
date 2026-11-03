// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#if !defined(UVNC_COMMON_H)
#define UVNC_COMMON_H

#include <commctrl.h>

namespace helper {

template<typename T> inline T *SafeGetWindowUserData(HWND hwnd)
{
    T *pUserData;
#ifndef _X64
	pUserData = (T *) GetWindowLong(hwnd, GWL_USERDATA);
#else
	pUserData = (T *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif

    return pUserData;
}

void SafeSetWindowUserData(HWND hwnd, LPARAM lParam);

// DWL_MSGRESULT
void SafeSetMsgResult(HWND hwnd, LPARAM result);
// GWL_HINSTANCE
HINSTANCE SafeGetWindowInstance(HWND hWnd);
// GWL_WNDPROC
LONG SafeGetWindowProc(HWND hWnd);
void SafeSetWindowProc(HWND hWnd, LONG_PTR pWndProc);

bool yesnoUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, char* szHeader, char* body, char* okStr, char* cancelStr, char* checkbox, BOOL& bCheckboxChecked);
bool yesUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, char* body, char* szHeader, int icon);
// Unicode overloads
bool yesnoUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, wchar_t* szHeader, wchar_t* body, wchar_t* okStr, wchar_t* cancelStr, wchar_t* checkbox, BOOL& bCheckboxChecked);
bool yesUVNCMessageBox(HINSTANCE hInst, HWND m_hWnd, wchar_t* body, wchar_t* szHeader, int icon);

void close_handle(HANDLE& h);

// Moves all contents of srcDir into dstDir, overwriting same-named files
// and merging subdirectories recursively. dstDir is created if needed.
// On full success srcDir is removed. Returns false if anything failed
// (unmoved items remain in srcDir).
bool MoveDirContentsInto(LPCWSTR srcDir, LPCWSTR dstDir);

// Returns true when 'name' is a single, plain file/folder name that cannot
// traverse into or alias another path: non-empty, contains no path
// separators, drive specifiers or other characters that are illegal in
// Windows file names, is not "." or "..", and does not end in a dot or
// space (Windows silently strips those, which would alias a sibling).
// Use on names received from a remote peer before using them in a path.
bool IsSafeFileNameW(LPCWSTR name);

// Returns true when 'name' is a relative path that cannot escape the
// directory it is joined to: not empty, not rooted or absolute, no drive
// specifier or ':' anywhere (also blocks ADS names), and no "." or ".."
// path components. Use on zip entry names or relative paths received from
// a remote peer.
bool IsSafeRelativePathW(LPCWSTR name);

// Returns true when 'path' resolves (via GetFullPathNameW) to a direct
// child of 'parentDir'. 'parentDir' may include a trailing backslash.
bool IsDirectChildPathW(LPCWSTR parentDir, LPCWSTR path);

// Returns true when 'szReceived' is the legitimate reply to a "[ name ]"
// directory request, i.e. a "<dir>\!UVNCDIR-<name>.zip" file. For
// Unicode-capable servers (fUnicodeWireNames) the name part is hex-encoded
// UTF-8 of the requested directory name and must decode to (a prefix of -
// senders truncate the hex at 200 chars) that name. For old servers a
// plain (8.3) name is accepted when it is a safe folder name. fOldProtocol
// also accepts the "(name)" request marker used with <= RC18 servers.
bool IsExpectedDirZipReply(const char* szReceived, const char* szRequested, bool fUnicodeWireNames, bool fOldProtocol);

    class DynamicFnBase {
    public:
      DynamicFnBase(const TCHAR* dllName, const char* fnName);
      ~DynamicFnBase();
      bool isValid() const {return fnPtr != 0;}
    protected:
      void* fnPtr;
      HMODULE dllHandle;
    };

    template<typename T> class DynamicFn : public DynamicFnBase {
    public:
      DynamicFn(const TCHAR* dllName, const char* fnName) : DynamicFnBase(dllName, fnName) {}
      T operator *() const {return (T)fnPtr;};
    };

}

#endif
