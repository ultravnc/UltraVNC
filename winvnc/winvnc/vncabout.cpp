/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
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


// vncAbout.cpp

// Implementation of the About dialog!

#include "stdhdrs.h"
#include "../common/Hyperlinks.h"
#include "winvnc.h"
#include "vncabout.h"
extern char configFile[256];

//	[v1.0.2-jp1 fix] Load resouce from dll
extern HINSTANCE	hInstResDLL;

char* GetVersionFromResource(char* version)
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

                    if (VerQueryValue(pResCopy, ("\\"), (LPVOID*)&lpFfi, &uLen))
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
    //strcat(version, (char*)"-dev");
    return version;
}


HBITMAP
    DoGetBkGndBitmap(IN CONST UINT uBmpResId )
    {
        static HBITMAP hbmBkGnd = NULL;
        if (NULL == hbmBkGnd)
        {
            /*char szFileName[MAX_PATH];
			if (GetModuleFileName(NULL, szFileName, MAX_PATH))
				{
				char* p = strrchr(szFileName, '\\');
					if (p == NULL) return false;
					*p = '\0';
				strcat_s(szFileName,"\\background2.bmp");
			}
			hbmBkGnd = (HBITMAP)LoadImage( NULL, szFileName, IMAGE_BITMAP, 0, 0,
               LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE );*/
			hbmBkGnd = (HBITMAP)LoadImage(
                GetModuleHandle(NULL), MAKEINTRESOURCE(uBmpResId),
                    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

            if (NULL == hbmBkGnd)
                hbmBkGnd = (HBITMAP)-1;
        }
        return (hbmBkGnd == (HBITMAP)-1)
            ? NULL : hbmBkGnd;
    }
BOOL
    DoSDKEraseBkGnd(
        IN CONST HDC hDC,
        IN CONST COLORREF crBkGndFill
      )
    {
        HBITMAP hbmBkGnd = DoGetBkGndBitmap(IDB_BITMAP1);
        if (hDC && hbmBkGnd)
        {
            RECT rc;
            if ((ERROR != GetClipBox(hDC, &rc)) && !IsRectEmpty(&rc))
            {
                HDC hdcMem = CreateCompatibleDC(hDC);
                if (hdcMem)
                {
                    HBRUSH hbrBkGnd = CreateSolidBrush(crBkGndFill);
                    if (hbrBkGnd)
                    {
                        HGDIOBJ hbrOld = SelectObject(hDC, hbrBkGnd);
                        if (hbrOld)
                        {
                            SIZE size = {
                                (rc.right-rc.left), (rc.bottom-rc.top)
                            };

                            if (PatBlt(hDC, rc.left, rc.top, size.cx, size.cy, PATCOPY))
                            {
                                HGDIOBJ hbmOld = SelectObject(hdcMem, hbmBkGnd);
                                if (hbmOld)
                                {
                                    BitBlt(hDC, rc.left, rc.top, size.cx, size.cy,
                                        hdcMem, rc.left, rc.top, SRCCOPY);
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

// Constructor/destructor
vncAbout::vncAbout()
{
	m_dlgvisible = FALSE;
}

vncAbout::~vncAbout()
{
}

// Initialisation
BOOL
vncAbout::Init()
{
	return TRUE;
}

// Dialog box handling functions
void
vncAbout::Show(BOOL show)
{
	if (show)
	{
		if (!m_dlgvisible)
		{
			//	[v1.0.2-jp1 fix]
			//DialogBoxParam(hAppInstance,
			DialogBoxParam(hInstResDLL,
				MAKEINTRESOURCE(IDD_ABOUT), 
				NULL,
				(DLGPROC) DialogProc,
				(LONG_PTR) this);
		}
	}
}

BOOL CALLBACK
vncAbout::DialogProc(HWND hwnd,
					 UINT uMsg,
					 WPARAM wParam,
					 LPARAM lParam )
{
	// We use the dialog-box's USERDATA to store a _this pointer
	// This is set only once WM_INITDIALOG has been recieved, though!
#ifndef _X64
	vncAbout *_this = (vncAbout *) GetWindowLong(hwnd, GWL_USERDATA);
#else
	vncAbout *_this = (vncAbout *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif
	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
            HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_WINVNC));
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			// Retrieve the Dialog box parameter and use it as a pointer
			// to the calling vncProperties object
#ifndef _X64
			SetWindowLong(hwnd, GWL_USERDATA, lParam);
#else
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
#endif
			_this = (vncAbout *) lParam;

			// Insert the build time information
			extern char buildtime[];
			SetDlgItemText(hwnd, IDC_BUILDTIME, buildtime);
            ConvertStaticToHyperlink(hwnd, IDC_WWW);
			// Show the dialog
			SetForegroundWindow(hwnd);

			_this->m_dlgvisible = TRUE;
            char version[50]{};
			char title[256]{};
			strcpy_s(title, "UltraVNC Server -");
			strcat_s(title, GetVersionFromResource(version));
#ifndef _X64
            strcat_s(title, " - x86");
#else
            strcat_s(title, " - x64");
#endif
			SetDlgItemText(hwnd, IDC_VERSION, title);

            const long lszConfigFileSize = 256;
            char szConfigFile[lszConfigFileSize];

            _snprintf_s(szConfigFile, lszConfigFileSize - 1, "Config file: %s", configFile);
            SetDlgItemText(hwnd, IDC_CONFIG_FILE, szConfigFile);
			return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{

		case IDCANCEL:
		case IDOK:
			// Close the dialog
			EndDialog(hwnd, TRUE);

			_this->m_dlgvisible = FALSE;

			return TRUE;
		}
        case IDC_WWW:
        {
            ShellExecute(GetDesktopWindow(), "open", "https://uvnc.com/", "", 0, SW_SHOWNORMAL);
            return TRUE;
        }

		break;

	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		_this->m_dlgvisible = FALSE;
		return TRUE;
	/*case WM_ERASEBKGND:
            {
                DoSDKEraseBkGnd((HDC)wParam, RGB(255,0,0));
				return true;
            }
	case WM_CTLCOLORSTATIC:
   {

    //GetStockObject(NULL_BRUSH);
    SetBkMode((HDC) wParam, TRANSPARENT);
	return (DWORD) GetStockObject(NULL_BRUSH);

   }*/
	}
	return 0;
}
