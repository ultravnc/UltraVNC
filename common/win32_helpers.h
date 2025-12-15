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

void close_handle(HANDLE& h);

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
