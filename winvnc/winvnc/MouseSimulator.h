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

class CursorColorManager {
private:
	HICON hCorsor[9];
	bool usedCursor[9];
	static HINSTANCE hInst;
	static CursorColorManager* instance;

public:
	CursorColorManager();
	~CursorColorManager();
	void Init(HINSTANCE hInst);
	HICON getCursor();
	void releaseCursor(HICON icon);
	HICON getEraser();
	static CursorColorManager* getInstance();
};

class SimulateCursor {
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK realWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool drawn = false;
	int oldx = 0;
	int oldy = 0;
	int x = 0;
	int y = 0;
	HICON hIconMouse;
	HICON hIconErase;
	HWND hWnd;
	HINSTANCE hInst;
	static DWORD WINAPI Start(LPVOID lpParam);
	static HWND create_window(SimulateCursor* simulateCursor);

public:
	SimulateCursor(HINSTANCE hInst);
	~SimulateCursor();
	void moveCursor(int x, int y);

};
