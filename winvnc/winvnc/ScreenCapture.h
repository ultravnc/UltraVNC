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
#include "common/VersionHelpers.h"

#define MAXCHANGES_BUF 2000
#define OSWIN10 10
#define OSVISTA 6
#define OSWINXP 5
#define OSOLD 4

#define SCREEN_SCREEN 11
#define BLIT 12
#define SOLIDFILL 13
#define BLEND 14
#define TRANS 15
#define PLG 17
#define TEXTOUT 18
#define POINTERCHANGE 19

#ifndef _WIN32_WINNT_WINTHRESHOLD
VERSIONHELPERAPI IsWindows10OrGreater();
#endif

typedef struct _CHANGES_RECORD
{
	ULONG type;  //screen_to_screen, blit, newcache,oldcache
	RECT rect;
	POINT point;
}CHANGES_RECORD;
typedef CHANGES_RECORD *PCHANGES_RECORD;
typedef struct _CHANGES_BUF
{
	ULONG counter;
	CHANGES_RECORD pointrect[MAXCHANGES_BUF];
}CHANGES_BUF;
typedef CHANGES_BUF *PCHANGES_BUF;


class ScreenCapture
{
public:
	ScreenCapture();
	virtual ~ScreenCapture() { ; }
	PCHAR getFramebuffer() { return pFramebuffer; }
	PCHANGES_BUF getChangeBuffer() { return pChangebuf; }
	int getPreviousCounter() { return oldAantal; }
	void setPreviousCounter(int oldAantal) { this->oldAantal = oldAantal; }
	void setBlocked(bool blocked) { this->blocked = blocked; }
	bool getBlocked() { return blocked; }

	virtual void videoDriver_start(int x, int y, int w, int h, bool onlyVirtual, int maxFPS) = 0;
	virtual void videoDriver_Stop() = 0;
	virtual bool hardwareCursor() = 0;
	virtual bool noHardwareCursor() = 0;
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
	virtual HANDLE getHScreenEvent() = 0;
	virtual HANDLE getHPointerEvent() = 0;

protected:
	int osVersion();
	int osVer;
	PCHAR pSharedMemory;
	PCHAR pFramebuffer;
	PCHANGES_BUF pChangebuf;
	ULONG oldAantal;
	bool blocked;
	bool init;	
};

