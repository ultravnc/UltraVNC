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

	virtual void videoDriver_start(int x, int y, int w, int h) = 0;
	virtual void videoDriver_Stop() = 0;
	virtual BOOL hardwareCursor() = 0;
	virtual BOOL noHardwareCursor() = 0;

protected:
	int osVersion();
	int osVer;
	PCHAR pSharedMemory;
	PCHAR pFramebuffer;
	PCHANGES_BUF pChangebuf;
	ULONG oldAantal;
	BOOL blocked;
	bool init;	
};

