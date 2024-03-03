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


#include "stdhdrs.h"
#include "Snapshot.h"
#include "vncviewer.h"
#include <windows.h>
#include <string>
#include <shlobj.h>
#include <iostream>
#include <sstream>
#include "../common/win32_helpers.h"
#include <gdiplus.h>

#pragma comment( lib, "gdiplus" )

static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM lpData)
{

    if(uMsg == BFFM_INITIALIZED)
    {
        std::string tmp = (const char *) lpData;
        std::cout << "path: " << tmp << std::endl;
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}

TCHAR * BrowseFolder(TCHAR * saved_path, HWND hwnd)
{
    static TCHAR path[MAX_PATH];

    const char * path_param = saved_path;

    BROWSEINFO bi = { 0 };
    bi.lpszTitle  = ("Browse for folder...");
    bi.ulFlags    = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn       = BrowseCallbackProc;
    bi.lParam     = (LPARAM) path_param;
	bi.hwndOwner = hwnd;

    LPITEMIDLIST pidl = SHBrowseForFolder ( &bi );

    if ( pidl != 0 )
    {
        //get the name of the folder and put it in path
        SHGetPathFromIDList ( pidl, path );

        //free memory used
        IMalloc * imalloc = 0;
        if ( SUCCEEDED( SHGetMalloc ( &imalloc )) )
        {
            imalloc->Free ( pidl );
            imalloc->Release ( );
        }
        return path;
    }
    return  saved_path;
}
std::string datetime()
{
    time_t rawtime;
    struct tm * timeinfo;
    TCHAR buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    _tcsftime(buffer,80,"%d-%m-%Y %H-%M-%S",timeinfo);



    return std::string(buffer);
}

void Snapshot::SaveJpeg(HBITMAP membit,TCHAR folder[MAX_PATH], TCHAR prefix[56], TCHAR imageFormat[56])
{
	_tcscpy_s(m_folder,  folder);
	_tcscpy_s(m_prefix,  prefix);
	if (strlen(m_folder) == 0 || strlen(m_prefix) == 0)
		DoDialogSnapshot(m_folder, m_prefix);

	TCHAR filename[MAX_PATH];
	TCHAR expanded_filename[MAX_PATH];
	_tcscpy_s(filename, m_folder);
	_tcscat_s(filename, "\\");
	_tcscat_s(filename, m_prefix); 

	 time_t rawtime;
    struct tm * timeinfo;
    TCHAR buffer[80];
    time (&rawtime);
    timeinfo = localtime(&rawtime);
    _tcsftime(buffer,80,"%Y%m%d_%H%M%S",timeinfo);
	_tcscat_s(filename, "_");
	_tcscat_s(filename, buffer);
	_tcscat_s(filename, imageFormat);
	ExpandEnvironmentStrings(filename, expanded_filename, MAX_PATH);
	using namespace Gdiplus;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
	{		
		Gdiplus::Bitmap bitmap(membit, NULL);
		CLSID clsid;
		if (strcmp(imageFormat, ".jpeg") == 0)
			GetEncoderClsid(L"image/jpeg", &clsid);
		else if (strcmp(imageFormat, ".png") == 0)
			GetEncoderClsid(L"image/png", &clsid);
		else if (strcmp(imageFormat, ".gif") == 0)
			GetEncoderClsid(L"image/gif", &clsid);
		else if (strcmp(imageFormat, ".bmp") == 0)
			GetEncoderClsid(L"image/bmp", &clsid);

		WCHAR wc[MAX_PATH];
		mbstowcs(wc, expanded_filename, MAX_PATH);
		bitmap.Save(wc, &clsid, NULL);
	}
	GdiplusShutdown(gdiplusToken);
}

int Snapshot::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	using namespace Gdiplus;
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes
	ImageCodecInfo* pImageCodecInfo = NULL;
	GetImageEncodersSize(&num, &size);
	if(size == 0)
		return -1;  // Failure
	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
		return -1;  // Failure
	GetImageEncoders(num, size, pImageCodecInfo);
	for(UINT j = 0; j < num; ++j)
	{
		if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}    
	}
	free(pImageCodecInfo);
	return 0;
}

Snapshot::Snapshot()
{
	_tcscpy_s(m_folder, "");
	_tcscpy_s(m_prefix, "");
}

Snapshot::~Snapshot()
{
}

int Snapshot::DoDialogSnapshot(TCHAR folder[MAX_PATH], TCHAR prefix[56])
{
	_tcscpy_s(m_folder,  folder);
	_tcscpy_s(m_prefix,  prefix);
	return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_SAVEIMAGE), NULL, (DLGPROC) DlgProc, (LONG_PTR) this);
}

BOOL CALLBACK Snapshot::DlgProc(  HWND hwnd,  UINT uMsg,  
									   WPARAM wParam, LPARAM lParam ) 
{
    Snapshot *_this = helper::SafeGetWindowUserData<Snapshot>(hwnd); 

	switch (uMsg) {

	case WM_INITDIALOG:
		{
            helper::SafeSetWindowUserData(hwnd, lParam);
			_this = (Snapshot *) lParam;
			SetDlgItemText(hwnd, IDC_FOLDER, _this->m_folder);
			SetDlgItemText(hwnd, IDC_PREFIX, _this->m_prefix);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				UINT res= GetDlgItemText( hwnd,  IDC_FOLDER,
					_this->m_folder, 256);
				res= GetDlgItemText( hwnd,  IDC_PREFIX,
					_this->m_prefix, 256);
				EndDialog(hwnd, TRUE);
				return TRUE;
			}
		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		case IDC_BROWSE:
			_tcscpy_s(_this->m_folder,MAX_PATH ,BrowseFolder(_this->m_folder, hwnd));
			SetDlgItemText(hwnd, IDC_FOLDER, _this->m_folder);
			return TRUE;
		}

		break;
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return 0;
}