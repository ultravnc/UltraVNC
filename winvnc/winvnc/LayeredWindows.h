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
	bool SetBlankMonitor(bool enabled, bool blankMonitorEnabled, bool black_window_activ);
	void SetBorderWindow(bool enabled, RECT rect, char* infoMsg, bool set_OSD);
};