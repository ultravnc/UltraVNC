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