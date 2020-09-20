#pragma once
#ifndef tempdisplayH
#define tempdisplayH
#include <stdio.h>
#include <WinSock2.h>
#include <Windows.h>
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