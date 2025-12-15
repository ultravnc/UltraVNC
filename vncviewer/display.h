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
#ifndef tempdisplayH
#define tempdisplayH
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <list>

typedef BOOL (WINAPI *ENUMDISPLAYSETTINGSEXA)(LPCSTR,DWORD,LPDEVMODEA,DWORD);
typedef LONG (WINAPI* pChangeDisplaySettingsExA)(LPCSTR,LPDEVMODEA,HWND,DWORD,LPVOID);
typedef struct _mymonitor
{
	int width;
	int height;
	int depth;
	int offsetx;
	int offsety;
	int freq;
	char devicename[100];
	char buttontext[250];
	int wl;
	int wr;
	int wt;
	int wb;
	HMONITOR hm;
}mymonitor;

struct MyBitmapInfo
  {
    BITMAPINFOHEADER bi;
    union
    {
      RGBQUAD colors[256];
      DWORD fields[256];
    };
  };

class tempdisplayclass
{
public:
	tempdisplayclass();
	~tempdisplayclass();
	void Init();
	void checkmonitors();
	int nr_monitors;
	mymonitor monarray[15];
	int getSelectedScreen(HWND hwnd , bool allowMonitorSpanning);

private:
	HINSTANCE hUser32;
};
#endif