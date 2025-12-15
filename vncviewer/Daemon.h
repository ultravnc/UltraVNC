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
// Phil Money @ Advantig, LLC 7-9-2005
static bool ListenMode = true; 
void SetListenMode(bool listenmode); 
bool GetListenMode(); 


class Daemon  
{
public:
	Daemon(int port, bool ipv6);
	virtual ~Daemon();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
protected:
	void AddTrayIcon();
	void CheckTrayIcon();
	void RemoveTrayIcon();
	bool SendTrayMsg(DWORD msg);
	bool ipv6;
	SOCKET m_deamon_sock6 = INVALID_SOCKET;
	SOCKET m_deamon_sock4 = INVALID_SOCKET;
	SOCKET m_deamon_sock = INVALID_SOCKET;

	HWND m_hwnd;
	HMENU m_hmenu;
	UINT m_timer;
	NOTIFYICONDATA m_nid;
	int m_nPort; // sf@2003
	char netbuf[1024];
};

