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


#pragma once
#include "VNCOptions.h"

class Snapshot 
{
private:
	VNCOptions m_opts;
	
	static BOOL CALLBACK DlgProc(  HWND hwndDlg,  UINT uMsg, WPARAM wParam, LPARAM lParam );
	TCHAR m_folder[MAX_PATH];
	TCHAR m_prefix[56];	
	int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);	
public:
	Snapshot();
	virtual ~Snapshot();
	int DoDialogSnapshot(TCHAR folder[MAX_PATH], TCHAR prefix[56]);
	TCHAR *getFolder(){return m_folder;}
	TCHAR *getPrefix(){return m_prefix;}
	void SaveJpeg(HBITMAP membit,TCHAR folder[MAX_PATH], TCHAR prefix[56], TCHAR format[56]);
};
