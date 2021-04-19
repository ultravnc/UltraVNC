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
