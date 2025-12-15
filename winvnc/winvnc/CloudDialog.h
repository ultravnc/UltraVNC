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
#include "common/inifile.h"
class vncServer;

class CloudDialog
{
private:
	vncServer* m_server = NULL;
	void SaveToIniFile();
	bool SC = false;
public:
	CloudDialog();
	~CloudDialog();
	void LoadFromIniFile();
	void Init(vncServer* server);
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Show(bool show, bool SC = false);
	bool m_dlgvisible;
};

