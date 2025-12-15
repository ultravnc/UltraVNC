// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "winvnc.h"
#include "vncserver.h"
#include "vncdesktop.h"
#include <string.h>
#include "uvncUiAccess.h"
#include "vncOSVersion.h"
#include "SettingsManager.h"

#if !defined(WM_DPICHANGED)
#define WM_DPICHANGED       0x02E0
#endif

int OSversion();
DWORD WINAPI InitWindowThread(LPVOID lpParam);
extern wchar_t g_hookstring[16];
extern int g_lockcode;


void
vncDesktop::ShutdownInitWindowthread()
{
	// we keep the sink window running
	// but ignore info
	can_be_hooked=false;
	g_lockcode = 0;
	vnclog.Print(LL_INTINFO, VNCLOG("ShutdownInitWindowthread \n"));
}

void
vncDesktop::StopInitWindowthread()
{
	//vndesktopthread is closing, all threads need to be stopped
	//else UltraVNC Server will stay running in background on exit
	g_lockcode = 0;
		can_be_hooked=true;
		if (InitWindowThreadh)
		{
			PostThreadMessage(pumpID, WM_QUIT, 0, 0);
			DWORD status=WaitForSingleObject(InitWindowThreadh,2000);
			if (status==WAIT_TIMEOUT)
			{
				vnclog.Print(LL_INTERR, VNCLOG("~vncDesktop::ERROR:  messageloop blocked \n"));
				// WE need to kill the thread to prevent a UltraVNC Server lock
				TerminateThread(InitWindowThreadh,0);
				CloseHandle(InitWindowThreadh);
				m_hwnd=NULL;
				InitWindowThreadh=NULL;
			}
			else
			{
				vnclog.Print(LL_INTERR, VNCLOG("~vncDesktop:: iniwindowthread proper closed \n"));
				CloseHandle(InitWindowThreadh);
				InitWindowThreadh=NULL;
			}
		}
		else
		{
			vnclog.Print(LL_INTINFO, VNCLOG("initwindowthread already closed \n"));
		}		
}

void
vncDesktop::StartInitWindowthread()
{
	// Check if the input desktop == Default desktop
	// Hooking the winlogon is not needed, no clipboard
	// see if the threaddesktop== Default
	HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;
	char new_name[256];
	can_be_hooked=false;
	if (GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		if (strcmp(new_name,"Default")==0)
		{
			if (InitWindowThreadh==NULL)
			{
				ResetEvent(restart_event);
				if (settings->getEnableWin8Helper())
					keybd_initialize();
				InitWindowThreadh=CreateThread(NULL,0,InitWindowThread,this,0,&pumpID);
				DWORD status=WaitForSingleObject(restart_event,10000);
				if (status==WAIT_TIMEOUT)
				{
					vnclog.Print(LL_INTINFO, VNCLOG("ERROR: initwindowthread failed to start \n"));
					if (InitWindowThreadh!=NULL)
					{
						TerminateThread(InitWindowThreadh,0);
						CloseHandle(InitWindowThreadh);
						m_hwnd=NULL;
						InitWindowThreadh=NULL;
					}
					can_be_hooked=false;
				}
				else
				{
					can_be_hooked=true;
				}
			}
			else
			{
				// initwindowthread is still running
				// make it back active
				can_be_hooked=true;
			}
		}
	}
}

DWORD WINAPI
InitWindowThread(LPVOID lpParam)
{	
	vncDesktop *mydesk=(vncDesktop*)lpParam;
	//if (mydesk->m_server->Win8HelperEnabled()) 
		//keybd_initialize();
	mydesk->InitWindow();
	if (settings->getEnableWin8Helper())
		keybd_delete();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Window procedure for the Desktop window
LRESULT CALLBACK
DesktopWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
#ifndef _X64
	vncDesktop *_this = (vncDesktop*)GetWindowLong(hwnd, GWL_USERDATA);
#else
	vncDesktop *_this = (vncDesktop*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
#endif
	#ifdef _DEBUG
										OutputDevMessage("Message %i",iMsg );
										//vnclog.Print(LL_INTERR, VNCLOG("%i  \n"),iMsg);
			#endif
	switch (iMsg)
	{
	case WM_CREATE:
		vnclog.Print(LL_INTERR, VNCLOG("wmcreate  \n"));
		break;
	case WM_TIMER:
		if (wParam == 100 && _this->can_be_hooked)
		{

			KillTimer(hwnd, 100);
			if (_this->SetHook)
			{
				_this->SetHook(hwnd);
				vnclog.Print(LL_INTERR, VNCLOG("set SC hooks OK\n"));
				_this->m_hookinited = TRUE;
				if (_this->SetKeyboardFilterHooks) _this->SetKeyboardFilterHooks(_this->m_bIsInputDisabledByClient || settings->getDisableLocalInputs());
				if (_this->SetMouseFilterHooks) _this->SetMouseFilterHooks(_this->m_bIsInputDisabledByClient || settings->getDisableLocalInputs());
			}
			else if (_this->SetHooks)
			{
				if (!_this->SetHooks(
					GetCurrentThreadId(),
					RFB_SCREEN_UPDATE,
					RFB_COPYRECT_UPDATE,
					RFB_MOUSE_UPDATE, 0
				))
				{
					vnclog.Print(LL_INTERR, VNCLOG("failed to set system hooks\n"));
					// Switch on full screen polling, so they can see something, at least...
					settings->setPollFullScreen(TRUE);
					_this->m_hookinited = FALSE;
				}
				else
				{
					vnclog.Print(LL_INTERR, VNCLOG("set hooks OK\n"));
					_this->m_hookinited = TRUE;
					// Start up the keyboard and mouse filters
					if (_this->SetKeyboardFilterHook) _this->SetKeyboardFilterHook(_this->m_bIsInputDisabledByClient || settings->getDisableLocalInputs());
					if (_this->SetMouseFilterHook) _this->SetMouseFilterHook(_this->m_bIsInputDisabledByClient || settings->getDisableLocalInputs());
				}
			}
		}
		if (wParam==1001 && settings->getEnableWin8Helper())
			keepalive();
		if (wParam == 110) {
			_this->m_buffer.m_cursor_shape_cleared = TRUE;
			KillTimer(hwnd, 110);
		}
		break;
	case WM_MOUSESHAPE:
		if (_this->can_be_hooked)
		{
			SetEvent(_this->trigger_events[3]);
		}
		break;
	case WM_REQUESTMOUSESHAPE:
		SetTimer(hwnd, 110, 3000, NULL);
		break;
	case WM_HOOKCHANGE:
		if (wParam==1)
			{
				if (_this->m_hookinited==FALSE)
							SetTimer(hwnd,100,1000,NULL);
			}
		else if (wParam==2)
		{
			if (_this->m_hookinited)
				{
					if (_this->SetHook)
					{
						if (_this->SetKeyboardFilterHooks) _this->SetKeyboardFilterHooks( _this->m_bIsInputDisabledByClient || settings->getEnableWin8Helper());
						if (_this->SetMouseFilterHooks) _this->SetMouseFilterHooks( _this->m_bIsInputDisabledByClient || settings->getEnableWin8Helper());
					}
					else if (_this->SetHooks)
					{
						if (_this->SetKeyboardFilterHook) _this->SetKeyboardFilterHook( _this->m_bIsInputDisabledByClient || settings->getEnableWin8Helper());
						if (_this->SetMouseFilterHook) _this->SetMouseFilterHook( _this->m_bIsInputDisabledByClient || settings->getEnableWin8Helper());
					}
				}
		}
		else if (_this->m_hookinited)
			{
				_this->m_hookinited=FALSE;
				if (_this->UnSetHook)
				{
					vnclog.Print(LL_INTERR, VNCLOG("unset SC hooks OK\n"));
					_this->UnSetHook(hwnd);
				}
				else if (_this->UnSetHooks)
				{
				if(!_this->UnSetHooks(GetCurrentThreadId()) )
					vnclog.Print(LL_INTERR, VNCLOG("Unsethooks Failed\n"));
				else vnclog.Print(LL_INTERR, VNCLOG("Unsethooks OK\n"));
				}
			}
		return true;

	case WM_QUERYENDSESSION:
		return DefWindowProc(hwnd, iMsg, wParam, lParam);

	case WM_CLOSE:
		if (_this->m_hnextviewer!=NULL) ChangeClipboardChain(hwnd, _this->m_hnextviewer);
		_this->m_hnextviewer=NULL;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		KillTimer(hwnd, 100);
		if (_this->m_hnextviewer!=NULL) ChangeClipboardChain(hwnd, _this->m_hnextviewer);
		_this->m_hnextviewer=NULL;
		if (_this->m_hookinited)
			{
				if (_this->UnSetHook)
				{
					vnclog.Print(LL_INTERR, VNCLOG("unset SC hooks OK\n"));
					_this->UnSetHook(hwnd);
				}
				else if (_this->UnSetHooks)
				{
				if(!_this->UnSetHooks(GetCurrentThreadId()) )
					vnclog.Print(LL_INTERR, VNCLOG("Unsethooks Failed\n"));
				else vnclog.Print(LL_INTERR, VNCLOG("Unsethooks OK\n"));
				}
				_this->m_hookinited=FALSE;
			}
		vnclog.Print(LL_INTERR, VNCLOG("WM_DESTROY\n"));
		PostQuitMessage(0); 
		break;
	///ddihook
	case WM_SYSCOMMAND:
		// User has clicked an item on the tray menu
		switch (wParam)
		{
			case SC_MONITORPOWER:
				vnclog.Print(LL_INTINFO, VNCLOG("Monitor22 %i\n"),lParam);
		}
		vnclog.Print(LL_INTINFO, VNCLOG("Monitor3 %i %i\n"),wParam,lParam);
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	case WM_POWER:
	case WM_POWERBROADCAST:
		// User has clicked an item on the tray menu
		switch (wParam)
		{
			case SC_MONITORPOWER:
				vnclog.Print(LL_INTINFO, VNCLOG("Monitor222 %i\n"),lParam);
		}
		vnclog.Print(LL_INTINFO, VNCLOG("Power3 %i %i\n"),wParam,lParam);
		return DefWindowProc(hwnd, iMsg, wParam, lParam);

	case WM_COPYDATA:
        {
			PCOPYDATASTRUCT pMyCDS = (PCOPYDATASTRUCT) lParam;
			if (pMyCDS->dwData==112233)
			{
					DWORD mysize=pMyCDS->cbData;
					char mytext[1024];
					char *myptr;
					char split[4][6];
					strcpy_s(mytext,(LPCSTR)pMyCDS->lpData);
					myptr=mytext;
					for (DWORD j =0; j<(mysize/20);j++)
					{
						for (int i=0;i<4;i++)
							{
								strcpy_s(split[i],"     ");
								strncpy_s(split[i],myptr,4);
								myptr=myptr+5;
							}
						_this->UpdateFullScreen();
					}
			}
			//vnclog.Print(LL_INTINFO, VNCLOG("copydata\n"));
        }
			return 0;

	// GENERAL
	case WM_APP + 10:
		//ddEngine disconnected
		//we need to restart it again
		_this->m_displaychanged = TRUE;
		_this->m_hookdriver = true;
		if(_this->m_screenCapture)
			_this->m_screenCapture->setBlocked(true);
		break;
	case WM_APP + 11:
#ifdef _DEBUG
		OutputDevMessage("ddengine NotifyPointerChange()");
#endif
		SetEvent(_this->trigger_events[3]);
		break;
	case WM_APP + 12:
#ifdef _DEBUG
		OutputDevMessage("ddengine NotifyScreenChange()");
#endif
		SetEvent(_this->trigger_events[0]);
		break;
	case WM_DPICHANGED:
		_this->m_screensize_changed = true;
	case WM_DISPLAYCHANGE:			
		// The display resolution is changing
		// We must kick off any clients since their screen size will be wrong
		// WE change the clients screensize, if they support it.
		vnclog.Print(LL_INTERR, VNCLOG("WM_DISPLAYCHANGE\n"));
		// We First check if the Resolution changed is caused by a temp resolution switch
		// For a temp resolution we don't use the driver, to fix the Mirror Driver
		// to the new change, a resolution switch is needed, preventing screensaver locking.

		if (_this->m_screenCapture != NULL) // Video Driver active
		{
			if (!_this->m_screenCapture->getBlocked())
			{
				_this->m_displaychanged = TRUE;
				_this->m_hookdriver=true;
				_this->m_screenCapture->setBlocked(true);
				vnclog.Print(LL_INTERR, VNCLOG("Resolution switch detected, driver active\n"));
			}
			else
			{
				//Remove display change, cause by driver activation
				_this->m_screenCapture->setBlocked(false);
				vnclog.Print(LL_INTERR, VNCLOG("Resolution switch by driver activation removed\n"));
			}
		}
		else
		{
				_this->m_displaychanged = TRUE;
				_this->m_hookdriver=false;
				vnclog.Print(LL_INTERR, VNCLOG("Resolution switch detected, driver NOT active\n"));
		}
		return 0;

	case WM_SYSCOLORCHANGE:
	case WM_PALETTECHANGED:
		if (!_this->m_displaychanged)
		{
		// The palette colours have changed, so tell the server

		// Get the system palette
            // better to use the wrong colors than close the connection
		_this->SetPalette();

		// Update any palette-based clients, too
		//set to flase to avoid deadlock
		_this->m_server->UpdatePalette(false);
		}
		return 0;

		// CLIPBOARD MESSAGES

	case WM_CHANGECBCHAIN:
		// The clipboard chain has changed - check our nextviewer handle
		if ((HWND)wParam == _this->m_hnextviewer)
			_this->m_hnextviewer = (HWND)lParam;
		else
			if (_this->m_hnextviewer != NULL) {
				// adzm - 2010-07 - Fix clipboard hangs
				// use SendNotifyMessage instead of SendMessage so misbehaving or hung applications
				// won't cause our thread to hang.
				SendNotifyMessage(_this->m_hnextviewer,
							WM_CHANGECBCHAIN,
							wParam, lParam);
			}

		return 0;

	case WM_DRAWCLIPBOARD:
		// adzm - 2010-07 - Fix clipboard hangs
		if (_this->can_be_hooked && !_this->m_settingClipboardViewer)
		{
			// The clipboard contents have changed
			if((GetClipboardOwner() != _this->Window()) &&
				//_this->m_initialClipBoardSeen &&
				_this->m_clipboard_active && !_this->m_server->IsThereFileTransBusy())
			{
				// adzm - 2010-07 - Extended clipboard
				{
					// only need a window when setting clipboard data
					omni_mutex_lock l(_this->m_update_lock,277);
					_this->m_server->UpdateClipTextEx(NULL);
				}
			}

			//_this->m_initialClipBoardSeen = TRUE;
		}

		if (_this->m_hnextviewer != NULL)
		{
			// adzm - 2010-07 - Fix clipboard hangs
			// Pass the message to the next window in clipboard viewer chain.

			// use SendNotifyMessage instead of SendMessage so misbehaving or hung applications
			// won't cause our thread to hang.
			return SendNotifyMessage(_this->m_hnextviewer, WM_DRAWCLIPBOARD, wParam, lParam);
		}

		return 0;

	default:
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////

ATOM m_wndClass = INVALID_ATOM;

BOOL
vncDesktop::InitWindow()
{
	HDESK desktop;
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);

	if (desktop == NULL)
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:OpenInputdesktop Error \n"));

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;

	char new_name[256];

	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:!GetUserObjectInformation \n"));
	}

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("InitWindow:SelectHDESK:!SetThreadDesktop \n"));
	}
	
	ChangeWindowMessageFilter(RFB_SCREEN_UPDATE, MSGFLT_ADD);
	ChangeWindowMessageFilter(RFB_COPYRECT_UPDATE, MSGFLT_ADD);
	ChangeWindowMessageFilter(RFB_MOUSE_UPDATE, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_QUIT, MSGFLT_ADD);
	ChangeWindowMessageFilter(WM_SHUTDOWN, MSGFLT_ADD);

	if (m_wndClass == 0) {
		// Create the window class
		WNDCLASSEX wndclass;

		wndclass.cbSize			= sizeof(wndclass);
		wndclass.style			= 0;
		wndclass.lpfnWndProc	= &DesktopWndProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hAppInstance;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= NULL;
		wndclass.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName	= (const char *) NULL;
		wndclass.lpszClassName	= szDesktopSink;
		wndclass.hIconSm		= NULL;

		// Register it
		m_wndClass = RegisterClassEx(&wndclass);
		if (!m_wndClass) {
			vnclog.Print(LL_INTERR, VNCLOG("failed to register window class\n"));
			SetEvent(restart_event);
			return FALSE;
		}
	}

	// And create a window
	m_hwnd = CreateWindow(szDesktopSink,
				"WinVNC",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				400, 200,
				NULL,
				NULL,
				hAppInstance,
				NULL);

	if (m_hwnd == NULL) {
		vnclog.Print(LL_INTERR, VNCLOG("failed to create hook window\n"));
		SetEvent(restart_event);
		return FALSE;
	}
	SetTimer(m_hwnd,1001,1000,NULL);
	// Set the "this" pointer for the window
    helper::SafeSetWindowUserData(m_hwnd, (LONG_PTR)this);

	// Enable clipboard hooking
	// adzm - 2010-07 - Fix clipboard hangs
	m_settingClipboardViewer = true;
	m_hnextviewer = SetClipboardViewer(m_hwnd);
	m_settingClipboardViewer = false;		
	vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO load hookdll's\n"));
	////////////////////////
	hModuleVNCHook =NULL;
	char szCurrentDir[MAX_PATH];
	strcpy_s(szCurrentDir, winvncFolder);
	strcat_s(szCurrentDir, "\\vnchooks.dll");
	hSCModule=NULL;
	char szCurrentDirSC[MAX_PATH];
	strcpy_s(szCurrentDirSC, winvncFolder);
#ifdef _X64
			strcat_s(szCurrentDirSC,"\\schook_legacy64.dll");
#else
			strcat_s(szCurrentDirSC,"\\schook_legacy.dll");
#endif


	UnSetHooks=NULL;
	SetMouseFilterHook=NULL;
	SetKeyboardFilterHook=NULL;
	SetMouseFilterHooks=NULL;
	SetKeyboardFilterHooks=NULL;
	SetHooks=NULL;

	UnSetHook=NULL;
	SetHook=NULL;

	hModuleVNCHook = LoadLibrary(szCurrentDir);
	hSCModule = LoadLibrary(szCurrentDirSC);//TOFIX resource leak
	if (hModuleVNCHook)
		{			
			UnSetHooks = (UnSetHooksFn) GetProcAddress(hModuleVNCHook, "UnSetHooks" );
			SetMouseFilterHook  = (SetMouseFilterHookFn) GetProcAddress(hModuleVNCHook, "SetMouseFilterHook" );
			SetKeyboardFilterHook  = (SetKeyboardFilterHookFn) GetProcAddress(hModuleVNCHook, "SetKeyboardFilterHook" );
			SetHooks  = (SetHooksFn) GetProcAddress(hModuleVNCHook, "SetHooks" );
		}
	if (hSCModule)
		{
			UnSetHook = (UnSetHookFn) GetProcAddress( hSCModule, "UnSetHook" );
			SetHook  = (SetHookFn) GetProcAddress( hSCModule, "SetHook" );
			SetMouseFilterHooks  = (SetMouseFilterHookFn) GetProcAddress( hSCModule, "SetMouseFilterHook" );
			SetKeyboardFilterHooks  = (SetKeyboardFilterHookFn) GetProcAddress( hSCModule, "SetKeyboardFilterHook" );
		}

	///////////////////////////////////////////////
	vnclog.Print(LL_INTERR, VNCLOG("OOOOOOOOOOOO start dispatch\n"));
	MSG msg;
	SetEvent(restart_event);
	while (TRUE)
	{
		
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
		{

			if (msg.message==WM_QUIT || fShutdownOrdered)
				{
					DestroyWindow(m_hwnd);
					SetEvent(trigger_events[5]);
					break;
				}
			else if (msg.message==WM_SHUTDOWN)
				{
					DestroyWindow(m_hwnd);
					break;
				}
			else if (msg.message==RFB_SCREEN_UPDATE)
				{
					wcscpy_s(g_hookstring,L"vnchook");
					if (can_be_hooked)
					{
					rfb::Rect rect;
					rect.tl = rfb::Point((SHORT)LOWORD(msg.wParam), (SHORT)HIWORD(msg.wParam));
					rect.br = rfb::Point((SHORT)LOWORD(msg.lParam), (SHORT)HIWORD(msg.lParam));
					//Buffer coordinates
					rect.tl.x-=m_ScreenOffsetx;
					rect.br.x-=m_ScreenOffsetx;
					rect.tl.y-=m_ScreenOffsety;
					rect.br.y-=m_ScreenOffsety;

					rect = rect.intersect(m_Cliprect);
					if (!rect.is_empty())
						{
							while (lock_region_add) Sleep(5);
							rgnpump.assign_union(rect);
							SetEvent(trigger_events[1]);
						}
					}
				}
			else if (msg.message==RFB_MOUSE_UPDATE)
				{
					if (can_be_hooked)
					{
					SetCursor((HCURSOR) msg.wParam);
					SetEvent(trigger_events[2]);
					}
				}
			else
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
		}
		else WaitMessage();
	}
	while (g_lockcode != 0)
	{
		Sleep(100);
	}
	KillTimer(m_hwnd,1001);
	if (hModuleVNCHook)
		FreeLibrary(hModuleVNCHook);
	if (hSCModule)
		FreeLibrary(hSCModule);
	SetThreadDesktop(old_desktop);
    CloseDesktop(desktop);
	m_hwnd = NULL;
	return TRUE;
}