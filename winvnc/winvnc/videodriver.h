/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////
#include "stdhdrs.h"
#include<windows.h>
#include <stdio.h>
#include <process.h>
#include <winioctl.h>
#define DBG 1


#define MAXCHANGES_BUF 2000

#define CLIP_LIMIT          50

#define IGNORE 0
#define FROM_SCREEN 1
#define FROM_DIB 2
#define TO_SCREEN 3




#define SCREEN_SCREEN 11
#define BLIT 12
#define SOLIDFILL 13
#define BLEND 14
#define TRANS 15
#define PLG 17
#define TEXTOUT 18


#define BMF_1BPP       1L
#define BMF_4BPP       2L
#define BMF_8BPP       3L
#define BMF_16BPP      4L
#define BMF_24BPP      5L
#define BMF_32BPP      6L
#define BMF_4RLE       7L
#define BMF_8RLE       8L
#define BMF_JPEG       9L
#define BMF_PNG       10L


#define NOCACHE 1
#define OLDCACHE 2
#define NEWCACHE 3
#ifndef VIDEODRIVER
#define VIDEODRIVER

#define CDS_UPDATEREGISTRY  0x00000001
#define CDS_TEST            0x00000002
#define CDS_FULLSCREEN      0x00000004
#define CDS_GLOBAL          0x00000008
#define CDS_SET_PRIMARY     0x00000010
#define CDS_RESET           0x40000000
#define CDS_SETRECT         0x20000000
#define CDS_NORESET         0x10000000

#define SIOCTL_TYPE 40000
#define IOCTL_SIOCTL_METHOD_IN_DIRECT \
    CTL_CODE( SIOCTL_TYPE, 0x900, METHOD_IN_DIRECT, FILE_ANY_ACCESS  )
#define IOCTL_SIOCTL_METHOD_OUT_DIRECT \
    CTL_CODE( SIOCTL_TYPE, 0x901, METHOD_OUT_DIRECT , FILE_ANY_ACCESS  )
#define IOCTL_SIOCTL_METHOD_BUFFERED \
    CTL_CODE( SIOCTL_TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS  )
#define IOCTL_SIOCTL_METHOD_NEITHER \
    CTL_CODE( SIOCTL_TYPE, 0x903, METHOD_NEITHER , FILE_ANY_ACCESS  )

typedef BOOL (WINAPI* pEnumDisplayDevices)(PVOID,DWORD,PVOID,DWORD);
typedef LONG (WINAPI* pChangeDisplaySettingsExA)(LPCSTR,LPDEVMODEA,HWND,DWORD,LPVOID);

//*********************************************************************
// DONT TOUCH STRUCTURES/ SHOULD BE EXACTLE THE SAME IN kernel/app/video
//*********************************************************************
//*********************************************************************
//  RINGBUFFER FOR PASSING RECT CHANGES TO VNV
//*********************************************************************
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

typedef struct _GETCHANGESBUF
	{
	 PCHANGES_BUF buffer;
	 PVOID UserbufferBegin;
	 PVOID UserbufferEnd;
	}GETCHANGESBUF;
typedef GETCHANGESBUF *PGETCHANGESBUF;

class vncVideoDriver
{

// Fields
public:

// Methods
public:
	// Make the desktop thread & window proc friends

	vncVideoDriver();
	~vncVideoDriver();
	BOOL Activate_video_driver(bool auto,int x,int y, int w,int h);
	void DesActivate_video_driver();
	void StartMirroring();
	void StopMirroring();
	BOOL ExistMirrorDriver();
	BOOL HardwareCursor();
	BOOL NoHardwareCursor();
	GETCHANGESBUF * CreateCommunicationBuffer(int screensize);
	GETCHANGESBUF * RemoveCommunicationBuffer();
	BOOL IsMirrorDriverActive();
	BOOL GetDllProductVersion(char* dllName, char *vBuffer, int size);
	void UpdateCommunicationBuffer();

	ULONG oldaantal;
	HDC gdc;
	BOOL driver_succes;
	BOOL blocked;
	HDC GetDcMirror();
	BOOL Tempres();
	GETCHANGESBUF OutGetChangesbuf;
	HANDLE hFile;
	BOOL first;
	BOOL Temp_Resolution;
	
protected:
		
};
#endif