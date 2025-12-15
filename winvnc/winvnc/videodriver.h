// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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