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
#include "stdhdrs.h"

class DlgChangePassword
{
private:
	HWND DlgChangePasswordHwnd = NULL;
	char title[1024]{};
	int passwordSize;
	char passwd[1024];
public:
	DlgChangePassword();
	int ShowDlg(HWND parentWindow, char* title, int passwordSize);
	~DlgChangePassword();
	bool InitDialog(HWND hwnd);
	void onOK(HWND hwnd);
	char* getPassword();
};

