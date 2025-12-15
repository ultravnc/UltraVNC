// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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
