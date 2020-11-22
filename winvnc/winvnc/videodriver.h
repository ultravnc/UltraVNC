/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#ifndef _VIDEOD_H
#define _VIDEOD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include <winbase.h>
#include <winreg.h>
#include "ScreenCapture.h"

class VideoDriver : public ScreenCapture
{
public:
	VideoDriver();
	virtual ~VideoDriver();
	virtual void videoDriver_start(int x, int y, int w, int h, bool onlyVirtual, int maxFPS);
	virtual void videoDriver_Stop();
	virtual bool hardwareCursor();
	virtual bool noHardwareCursor();
	virtual void Lock(){};
	virtual void Unlock(){};
	virtual HANDLE getHScreenEvent(){return NULL;}
	virtual HANDLE getHPointerEvent(){return NULL;}

private:
	
	bool mirror_driver_attach_XP(int x, int y, int w, int h);
	void mirror_driver_detach_XP();
	bool mirror_driver_Vista(DWORD dwAttach, int x, int y, int w, int h);
	PCHAR videoMemory_GetSharedMemory(void);
	void videoMemory_ReleaseSharedMemory(PCHAR pVideoMemory);
	HDC getDcMirror();		
	DWORD shared_buffer_size;
};

#endif