
#pragma once
#include "stdhdrs.h"
#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_
#if _MSC_VER > 1000
#endif // _MSC_VER > 1000
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <winsock2.h>
#include <windows.h>
#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#include <time.h>
#include "stdhdrs.h"
#include "resource.h"
#include "vncservice.h"
#include "vncdesktop.h"
#include "vncdesktopthread.h"
#include "vncOSVersion.h"
#include "LayeredWindows.h"

HWND LayeredWindows::hwnd;
HINSTANCE LayeredWindows::hInst;
int LayeredWindows::wd;
int LayeredWindows::ht;
RECT LayeredWindows::rect;
HFONT LayeredWindows::hFont;
HPEN LayeredWindows::hPen;
char LayeredWindows::infoMsg[255] = { 0 };
bool LayeredWindows::set_OSD = false;

LayeredWindows::LayeredWindows()
{
   wd = 0;
   ht = 0;
   hwnd = NULL;
   hInst = NULL;
   HDC hDC = GetDC(NULL);
   nHeight = MulDiv(-18, GetDeviceCaps(hDC, LOGPIXELSY), 72);
   hPen = CreatePen(PS_DASHDOTDOT, 5, RGB(255, 0, 0));
   hFont = CreateFont(nHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, "Verdana");   
}
LayeredWindows::~LayeredWindows()
{
    DeleteObject(hFont);
    DeleteObject(hPen);
}

HBITMAP LayeredWindows::DoGetBkGndBitmap2(IN CONST UINT uBmpResId)
{
    static HBITMAP hbmBkGnd = NULL;
    if (NULL == hbmBkGnd)
    {
        char WORKDIR[MAX_PATH];
        char mycommand[MAX_PATH];
        if (GetModuleFileName(NULL, WORKDIR, MAX_PATH)) {
            char* p = strrchr(WORKDIR, '\\');
            if (p == NULL) return 0;
            *p = '\0';
        }
        strcpy_s(mycommand, WORKDIR);
        strcat_s(mycommand, "\\background.bmp");

        hbmBkGnd = (HBITMAP)LoadImage(NULL, mycommand, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (hbmBkGnd == NULL) {
            hbmBkGnd = (HBITMAP)LoadImage(
                GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO64), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        }
        BITMAPINFOHEADER h2;
        h2.biSize = sizeof(h2);
        h2.biBitCount = 0;
        HDC hxdc = CreateDC("DISPLAY", NULL, NULL, NULL);
        GetDIBits(hxdc, hbmBkGnd, 0, 0, NULL, (BITMAPINFO*)&h2, DIB_RGB_COLORS);
        wd = h2.biWidth; ht = h2.biHeight;
        DeleteDC(hxdc);
        if (NULL == hbmBkGnd)
            hbmBkGnd = (HBITMAP)-1;
    }
    return (hbmBkGnd == (HBITMAP)-1)
        ? NULL : hbmBkGnd;
}


BOOL LayeredWindows::DoSDKEraseBkGnd2(IN CONST HDC hDC, IN CONST COLORREF crBkGndFill)
{
    HBITMAP hbmBkGnd = DoGetBkGndBitmap2(0);
    if (hDC && hbmBkGnd)
    {
        RECT rc;
        if ((ERROR != GetClipBox(hDC, &rc)) && !IsRectEmpty(&rc)) {
            HDC hdcMem = CreateCompatibleDC(hDC);
            if (hdcMem) {
                HBRUSH hbrBkGnd = CreateSolidBrush(crBkGndFill);
                if (hbrBkGnd) {
                    HGDIOBJ hbrOld = SelectObject(hDC, hbrBkGnd);
                    if (hbrOld) {
                        SIZE size = { (rc.right - rc.left), (rc.bottom - rc.top) };
                        if (PatBlt(hDC, rc.left, rc.top, size.cx, size.cy, PATCOPY)) {
                            HGDIOBJ hbmOld = SelectObject(hdcMem, hbmBkGnd);
                            if (hbmOld) {
                                StretchBlt(hDC, 0, 0, size.cx, size.cy, hdcMem, 0, 0, wd, ht, SRCCOPY);
                                SelectObject(hdcMem, hbmOld);
                            }
                        }
                        SelectObject(hDC, hbrOld);
                    }
                    DeleteObject(hbrBkGnd);
                }
                DeleteDC(hdcMem);
            }
        }
    }
    return TRUE;
}

LRESULT CALLBACK  LayeredWindows::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 100, 20, NULL);
        break;
    case WM_TIMER:
        if (wParam == 100)
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    case WM_ERASEBKGND:
        DoSDKEraseBkGnd2((HDC)wParam, RGB(0, 0, 0));
        return true;
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wParam, TRANSPARENT);
        return (LONG_PTR)GetStockObject(NULL_BRUSH);
    case WM_DESTROY:
        KillTimer(hwnd, 100);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

bool LayeredWindows::create_black_window(void)
{
    WNDCLASSEX wndClass;
    ZeroMemory(&wndClass, sizeof(wndClass));
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInst;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hIconSm = NULL;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "blackscreen";
    RegisterClassEx(&wndClass);

    RECT clientRect;
    clientRect.left = 0;
    clientRect.top = 0;
    clientRect.right = GetSystemMetrics(SM_CXSCREEN);
    clientRect.bottom = GetSystemMetrics(SM_CYSCREEN);

    UINT x(GetSystemMetrics(SM_XVIRTUALSCREEN));
    UINT y(GetSystemMetrics(SM_YVIRTUALSCREEN));
    UINT cx(GetSystemMetrics(SM_CXVIRTUALSCREEN));
    UINT cy(GetSystemMetrics(SM_CYVIRTUALSCREEN));

    clientRect.left = x;
    clientRect.top = y;
    clientRect.right = x + cx;
    clientRect.bottom = y + cy;

    AdjustWindowRect(&clientRect, WS_CAPTION, FALSE);
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, "blackscreen", "blackscreen",
        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
        CW_USEDEFAULT, CW_USEDEFAULT, cx, cy, NULL, NULL, hInst, NULL);
    typedef DWORD(WINAPI* PSLWA)(HWND, DWORD, BYTE, DWORD);

    PSLWA pSetLayeredWindowAttributes = NULL;
    HMODULE hDLL = LoadLibrary("user32");
    if (hDLL) pSetLayeredWindowAttributes = (PSLWA)GetProcAddress(hDLL, "SetLayeredWindowAttributes");

#ifndef _X64
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_DLGFRAME | WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, style);
#else
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_DLGFRAME | WS_THICKFRAME);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
#endif

    if (pSetLayeredWindowAttributes != NULL) {
#ifndef _X64
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#else
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#endif
        ShowWindow(hwnd, SW_SHOWNORMAL);
    }
    if (pSetLayeredWindowAttributes != NULL)
        pSetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 255, LWA_ALPHA);
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, cx, cy, SWP_FRAMECHANGED | SWP_NOACTIVATE);
    if (VNC_OSVersion::getInstance()->OS_WIN10_TRANS)
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    return true;
}

DWORD WINAPI LayeredWindows::BlackWindow(LPVOID lpParam)
{
    HDESK desktop;
    desktop = OpenInputDesktop(0, FALSE,
        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
    );

    HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
    DWORD dummy{};

    char new_name[256]{};
    if (desktop) {
        GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy);
        SetThreadDesktop(desktop);
    }

    create_black_window();
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    vnclog.Print(LL_INTERR, VNCLOG("end BlackWindow \n"));
    SetThreadDesktop(old_desktop);
    if (desktop) CloseDesktop(desktop);

    return 0;
}

LRESULT CALLBACK LayeredWindows::WndBorderProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    switch (uMsg) {
    case WM_PAINT: {
        HGDIOBJ original = NULL;
        hdc = BeginPaint(hwnd, &ps);
        original = SelectObject(hdc, GetStockObject(DC_PEN));       
        SelectObject(hdc, hPen);
        SelectObject(hdc, hFont);

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
        SetTextColor(hdc, RGB(255, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        
        if (set_OSD) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            rc.left += 10;
            rc.top += 10;
            DrawText(hdc, infoMsg, strlen(infoMsg), &rc, DT_LEFT);
        }

        SelectObject(hdc, original);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
    }
                 break;
    case WM_CLOSE:        
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI LayeredWindows::BorderWindow(LPVOID lpParam)
{
    Sleep(1000);
    HDESK desktop;
    desktop = OpenInputDesktop(0, FALSE,
        DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
        DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
        DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
        DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
    );

    HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
    DWORD dummy{};

    char new_name[256]{};
    if (desktop) {
        GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy);
        SetThreadDesktop(desktop);
    }

    create_border_window(rect);
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    SetThreadDesktop(old_desktop);
    if (desktop) CloseDesktop(desktop);

    return 0;
}

bool LayeredWindows::create_border_window(RECT rect)
{    
    WNDCLASSEX wndClass;
    ZeroMemory(&wndClass, sizeof(wndClass));
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndBorderProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInst;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hIconSm = NULL;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "borderscreen";

    RegisterClassEx(&wndClass);
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, "borderscreen", "borderscreen",
        WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInst, NULL);
    typedef DWORD(WINAPI* PSLWA)(HWND, DWORD, BYTE, DWORD);

    PSLWA pSetLayeredWindowAttributes = NULL;
    HMODULE hDLL = LoadLibrary("user32");
    if (hDLL) pSetLayeredWindowAttributes = (PSLWA)GetProcAddress(hDLL, "SetLayeredWindowAttributes");

#ifndef _X64
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_DLGFRAME | WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, style);
#else
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_DLGFRAME | WS_THICKFRAME);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
#endif

    if (pSetLayeredWindowAttributes != NULL) {
#ifndef _X64
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#else
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#endif
        ShowWindow(hwnd, SW_SHOWNORMAL);
    }
    if (pSetLayeredWindowAttributes != NULL)
        pSetLayeredWindowAttributes(hwnd, RGB(255, 255, 255), 0, LWA_COLORKEY);
    SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);
    if (VNC_OSVersion::getInstance()->OS_WIN10_TRANS)
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
    return true;
}

bool LayeredWindows::SetBlankMonitor(bool enabled, bool blankMonitorEnabled, bool black_window_active)
{
    if (!VNC_OSVersion::getInstance()->OS_WIN10_TRANS && VNC_OSVersion::getInstance()->OS_WIN10
        || VNC_OSVersion::getInstance()->OS_WIN8)
        return false;

    // Also Turn Off the Monitor if allowed ("Blank Screen", "Blank Monitor")
    if (blankMonitorEnabled)
    {
        if (enabled) {
            if (VNC_OSVersion::getInstance()->OS_AERO_ON)
                VNC_OSVersion::getInstance()->DisableAero();

            HANDLE ThreadHandle2 = NULL;
            DWORD dwTId;
            ThreadHandle2 = CreateThread(NULL, 0, BlackWindow, NULL, 0, &dwTId);
            if (ThreadHandle2)
                CloseHandle(ThreadHandle2);
            black_window_active = true;
        }
        else {
            HWND Blackhnd = FindWindow(("blackscreen"), 0);
            if (Blackhnd)
                PostMessage(Blackhnd, WM_CLOSE, 0, 0);
            black_window_active = false;
            VNC_OSVersion::getInstance()->ResetAero();
        }
    }
    return black_window_active;
}

void LayeredWindows::SetBorderWindow(bool enabled, RECT rect, char* infoMsg, bool set_OSD)
{
    strcpy_s(this->infoMsg, infoMsg);
    this->rect = rect;
    this->set_OSD = set_OSD;
    if (enabled) {
        HANDLE ThreadHandle2 = NULL;
        DWORD dwTId;
        ThreadHandle2 = CreateThread(NULL, 0, BorderWindow, NULL, 0, &dwTId);
        if (ThreadHandle2)
            CloseHandle(ThreadHandle2);
    }
    else {
        HWND Blackhnd = FindWindow(("borderscreen"), 0);
        if (Blackhnd)
            PostMessage(Blackhnd, WM_CLOSE, 0, 0);
    }
}

