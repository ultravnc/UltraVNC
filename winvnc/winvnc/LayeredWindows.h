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


class LayeredWindows
{
private:
	static HWND hwnd;
	static HINSTANCE hInst;
	static int wd;
	static int ht;
	static HBITMAP DoGetBkGndBitmap2(IN CONST UINT uBmpResId);
	static BOOL DoSDKEraseBkGnd2(IN CONST HDC hDC, IN CONST COLORREF crBkGndFill);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static bool create_black_window(void);
	static LRESULT CALLBACK WndBorderProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static bool create_border_window(RECT rect);
	static DWORD WINAPI BorderWindow(LPVOID lpParam);
	static DWORD WINAPI BlackWindow(LPVOID lpParam);
	static RECT rect;
	static HFONT hFont;
	static HPEN hPen;
	static char infoMsg[255];
	static bool set_OSD;
	int nHeight;
public:
	LayeredWindows();
	~LayeredWindows();
	bool SetBlankMonitor(bool enabled, bool blankMonitorEnabled, bool black_window_activ, bool &screen_in_powersave, HWND hwnd);
	void SetBorderWindow(bool enabled, RECT rect, char* infoMsg, bool set_OSD);
};