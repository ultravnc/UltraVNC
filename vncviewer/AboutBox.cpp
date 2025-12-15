// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "../common/Hyperlinks.h"
#include "UltraVNCHelperFunctions.h"

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
				strcat (szFileName,"\\background2.bmp");
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
        HBITMAP hbmBkGnd = DoGetBkGndBitmap(IDB_BITMAP10);
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

void convertToISO8601(const char* input, char* output, size_t size) {
    // Expected format: "Mar 14 2025 12:34:56"

    // Convert month abbreviation to a number
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";

    char monthStr[4];  // Buffer for the month abbreviation (e.g., "Mar")
    int day, year, month;
    int hour, minute, second;

    // Extract components from the input string
    sscanf(input, "%3s %d %d %d:%d:%d", monthStr, &day, &year, &hour, &minute, &second);

    // Convert month abbreviation to number (1-12)
    month = (std::strstr(months, monthStr) - months) / 3 + 1;

    // Format into ISO 8601 format "YYYY-MM-DDTHH:MM:SS"
    snprintf(output, size, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}

// Process the About dialog.
static LRESULT CALLBACK AboutDlgProc(HWND hwnd, UINT iMsg, 
										   WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
	case WM_INITDIALOG:
		{
			//CentreWindow(hwnd);
			SetForegroundWindow(hwnd);
            extern char buildtime[];
            char isoTime[20];  // Buffer for ISO output
            convertToISO8601(buildtime, isoTime, sizeof(isoTime));
            SetDlgItemText(hwnd, IDC_BUILDTIME, isoTime);

            ConvertStaticToHyperlink(hwnd, IDC_UVNCCOM);
            char version[50]{};
            char title[256]{};
            strcpy_s(title, "UltraVNC Viewer - ");
            strcat_s(title, GetVersionFromResource(version));
            SetDlgItemText(hwnd, IDC_UVVERSION, title);

            HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

			return TRUE;
		}
	case WM_CLOSE:
		EndDialog(hwnd, TRUE);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			EndDialog(hwnd, TRUE);
		}
        if (LOWORD(wParam) == IDC_UVNCCOM) {
            ShellExecute(GetDesktopWindow(), "open", "https://uvnc.com/", "", 0, SW_SHOWNORMAL);
        }

	}
	return FALSE;
}

void ShowAboutBox()
{
	int res = DialogBox(pApp->m_instance, 
 		DIALOG_MAKEINTRESOURCE(IDD_APP_ABOUT),
		NULL, (DLGPROC) AboutDlgProc);
}
	
