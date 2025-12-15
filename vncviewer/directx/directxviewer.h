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
#include <winsock2.h>
#include <windows.h>
#include <d3d9.h>
#include "omnithread.h"
//#include <d3dx9.h>

typedef struct {

    unsigned char bitsPerPixel;
    unsigned char depth;
    unsigned char bigEndian;
    unsigned char trueColour;
    unsigned long redMask;
    unsigned long greenMask;
    unsigned long blueMask;
	unsigned long redShift;
    unsigned long greenShift;
    unsigned long blueShift;

} PixelFormat;

typedef IDirect3D9 * (__stdcall *D3DCREATETYPE)(UINT);

class ViewerDirectxClass
{
public:
	ViewerDirectxClass();
	virtual ~ViewerDirectxClass();
	HRESULT InitD3D(HWND hwnd, HWND hwndm, int width, int height, bool fullscreen,int bit,int shift);
	HRESULT ReInitD3D();
	void DestroyD3D(void);
	unsigned char *Preupdate(unsigned char * bits);
	unsigned char *Preupdate2(unsigned char * bits);
	bool Afterupdate();
	PixelFormat m_directxformat;
	bool paint();
	


private:
IDirect3D9* pD3D9;
IDirect3DDevice9* pD3DDevice9;
LPDIRECT3DSURFACE9 surface;
D3DPRESENT_PARAMETERS d3dpp;
D3DFORMAT format;
bool paintdevice();
bool valid();
bool directxlocked;
HWND parent_hwnd;
HWND hwnd_sendm;
int mwidth;
int mheight;
int mbit;
int mshift;
int m_nummer;
bool devicelost;
//////////////////////
HMODULE D3DLibrary;
D3DCREATETYPE d3dCreate;
omni_mutex directxMutex;

};