// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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
    //strcat(version, (char* )"-dev");
    return version;
}
