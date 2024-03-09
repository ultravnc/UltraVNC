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


#include "UltraVNCHelperFunctions.h"
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Version.lib")

char str50275[128];
char str50276[128];
char str50277[128];
char str50278[128];
char str50279[128];
char str50280[128];
char str50281[128];
char str50282[128];
char str50283[128];
char str50284[128];
char str50285[128];
char str50286[128];
char str50287[128];
char str50288[128];
char str50289[128];
char str50290[128];
char str50293[128];
char str50294[128];
char str50295[128];
char str50296[128];
char str50297[128];
extern HINSTANCE m_hInstResDLL;


void loadStrings(HINSTANCE m_hInstResDLL)
{
    LoadString(m_hInstResDLL, IDS_STRING50275, str50275, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50276, str50276, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50277, str50277, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50278, str50278, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50279, str50279, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50280, str50280, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50281, str50281, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50282, str50282, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50283, str50283, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50284, str50284, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50285, str50285, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50286, str50286, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50287, str50287, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50288, str50288, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50289, str50289, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50290, str50290, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50293, str50293, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50294, str50294, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50295, str50295, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50296, str50296, 128 - 1);
    LoadString(m_hInstResDLL, IDS_STRING50297, str50297, 128 - 1);
}

bool yesnoUVNCMessageBox(HWND m_hWnd, char* szHeader, char* body, char* okStr, char* cancelStr, char* checkbox, BOOL &bCheckboxChecked)
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
    LPCWSTR szTitle = L"UltraVNC Viewer";
    /*,
        szHeader = L"The server does not support encryption, despite the viewer's request for it.",
        szBodyText = L"Do you want to continue?",
        szCheckboxText = L"Don't ask anymore, always allow insecure connection.";*/
    TASKDIALOG_BUTTON aCustomButtons[] = {
        { 1000, w_okStr},
        { 1001, w_cancelStr}
    };
    tdc.cbSize = sizeof(tdc);
    tdc.hwndParent = m_hWnd;
    tdc.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_USE_COMMAND_LINKS;
    tdc.pButtons = aCustomButtons;
    tdc.cButtons = _countof(aCustomButtons);
    tdc.pszWindowTitle = szTitle;
    tdc.nDefaultButton = 1001;

    tdc.hInstance = m_hInstResDLL;
    tdc.pszMainIcon = MAKEINTRESOURCEW(IDR_TRAY);// TD_INFORMATION_ICON;
    tdc.pszMainInstruction = w_header;
    tdc.pszContent = w_body;
    if (strlen(checkbox) > 0)
        tdc.pszVerificationText = w_checkbox;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, &bCheckboxChecked);

    if (SUCCEEDED(hr) && 1000 == nClickedBtn)
        return true;
    return false;
}

void yesUVNCMessageBox(HWND m_hWnd, char* body, char* szHeader, int icon)
{
    wchar_t w_header[128];
    wchar_t w_body[1024];
    size_t outSize;

    mbstowcs_s(&outSize, w_header, szHeader, strlen(szHeader) + 1);
    mbstowcs_s(&outSize, w_body, body, strlen(body) + 1);

    HRESULT hr;
    TASKDIALOGCONFIG tdc = { sizeof(TASKDIALOGCONFIG) };
    int nClickedBtn;
    LPCWSTR szTitle = L"UltraVNC Viewer";


    tdc.hwndParent = m_hWnd;
    tdc.dwCommonButtons = TDCBF_YES_BUTTON;
    tdc.pszWindowTitle = szTitle;

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
    }
    tdc.pszMainInstruction = w_header;
    tdc.pszContent = w_body;

    hr = TaskDialogIndirect(&tdc, &nClickedBtn, NULL, NULL);
}

char * GetVersionFromResource(char *version)
{
    HRSRC hResInfo;
    DWORD dwSize;
    HGLOBAL hResData;
    LPVOID pRes, pResCopy;
    UINT uLen = 0;
    VS_FIXEDFILEINFO* lpFfi = NULL;
    HINSTANCE hInst = ::GetModuleHandle(NULL);

    hResInfo = FindResource(hInst, MAKEINTRESOURCE(1), RT_VERSION);
    if (hResInfo)
    {
        dwSize = SizeofResource(hInst, hResInfo);
        hResData = LoadResource(hInst, hResInfo);
        if (hResData)
        {
            pRes = LockResource(hResData);
            if (pRes)
            {
                pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
                if (pResCopy)
                {
                    CopyMemory(pResCopy, pRes, dwSize);

                    if (VerQueryValue(pResCopy, _T("\\"), (LPVOID*)&lpFfi, &uLen))
                    {
                        if (lpFfi != NULL)
                        {
                            DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
                            DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

                            DWORD dwLeftMost = HIWORD(dwFileVersionMS);
                            DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
                            DWORD dwSecondRight = HIWORD(dwFileVersionLS);
                            DWORD dwRightMost = LOWORD(dwFileVersionLS);

                            sprintf(version, " %d.%d.%d.%d", dwLeftMost, dwSecondLeft, dwSecondRight, dwRightMost);
                        }
                    }

                    LocalFree(pResCopy);
                }
            }
        }
    }
    return version;
}
