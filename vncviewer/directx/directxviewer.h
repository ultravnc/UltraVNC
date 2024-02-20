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
#include <winsock2.h>
#include <windows.h>
#include <d3d9.h>
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

};