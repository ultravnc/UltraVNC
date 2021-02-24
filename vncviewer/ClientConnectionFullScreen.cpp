//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uvnc.com or 
// contact the authors on vnc@uk.research.att.com for information on obtaining it.
//
// Many thanks to Greg Hewgill <greg@hewgill.com> for providing the basis for 
// the full-screen mode.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "ClientConnection.h"
#include "FullScreenTitleBar.h" //Added by: Lars Werner (http://lars.werner.no)
#include "display.h"

// Parameters for scrolling in full screen mode
#define BUMPSCROLLBORDER 8
#define BUMPSCROLLAMOUNTX 32
#define BUMPSCROLLAMOUNTY 8
extern char sz_J1[128];
extern char sz_J2[64];

void ClientConnection::saveScreenPosition()
{
	GetWindowRect(m_hwndMain, &mainRect);
}

void ClientConnection::restoreScreenPosition()
{
	SetWindowPos(m_hwndMain, HWND_NOTOPMOST, mainRect.left, mainRect.top, mainRect.right- mainRect.left, mainRect.bottom- mainRect.top, SWP_FRAMECHANGED);
}

bool ClientConnection::InFullScreenMode() 
{
	return m_opts.m_FullScreen; 
};

// You can explicitly change mode by calling this
void ClientConnection::SetFullScreenMode(bool enable)
{
	if (enable) {
		ShowToolbar = m_opts.m_ShowToolbar;
		m_opts.m_ShowToolbar = 0;		
		if (!m_opts.m_SavePos)
			saveScreenPosition();
		SizeWindow(enable);
		m_opts.m_FullScreen = enable;
		RealiseFullScreenMode();
	}
	else if (ShowToolbar != -1) {		
		m_opts.m_ShowToolbar = ShowToolbar;
		ShowToolbar = -1;		
		SizeWindow();	
		m_opts.m_FullScreen = enable;
		RealiseFullScreenMode();
		if (!m_opts.m_SavePos)
			restoreScreenPosition();
		if (extSDisplay)
			ScrollScreen(offsetXExtSDisplay, offsetYExtSDisplay, true);

	}

	SendFullFramebufferUpdateRequest(false);
    RedrawWindow(m_hwndMain, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
}

// If the options have been changed other than by calling 
// SetFullScreenMode, you need to call this to make it happen.
//void ofnInit();
void ClientConnection::RealiseFullScreenMode()
{
	if (m_opts.m_NoBorder) {
		BorderlessMode();
		return;
	}
	if (m_FullScreen != m_opts.m_FullScreen) {
		m_FullScreen = m_opts.m_FullScreen;
		m_fScalingDone = false;
	}
	HMONITOR hMonitor = ::MonitorFromWindow(m_hwndMain, MONITOR_DEFAULTTONEAREST);

	LONG style = GetWindowLong(m_hwndMain, GWL_STYLE);
	if (m_opts.m_FullScreen) {		
		style = GetWindowLong(m_hwndMain, GWL_STYLE);
		style &= ~(WS_CAPTION | WS_DLGFRAME | WS_THICKFRAME);
		style |= WS_MAXIMIZE |WS_POPUP;
		SetWindowLong(m_hwndMain, GWL_STYLE, style);

        // 7 May 2008 jdp
		RECT rect;
		GetWindowRect(m_hwndMain, &rect);
        MONITORINFO mi;
        mi.cbSize = sizeof (MONITORINFO);

        GetMonitorInfo(hMonitor, &mi);
        int x = mi.rcMonitor.left; 
        int y = mi.rcMonitor.top;
		int cx = mi.rcMonitor.right - x; 
		int cy = mi.rcMonitor.bottom - y;

		// when the remote size is bigger then 1,5 time the localscreen we use all monitors in
		// fullscreen mode
		if (m_opts.m_allowMonitorSpanning && !m_opts.m_showExtend){
			tempdisplayclass tdc;
			tdc.Init();
			x = 0;
			y = 0;
			cy = tdc.monarray[0].height;
			cx = tdc.monarray[0].width;
		}

		SetWindowPos(m_hwndMain, HWND_TOPMOST, x, y, cx, cy, SWP_FRAMECHANGED);
        TitleBar.MoveToMonitor(hMonitor);
		// adzm - 2010-07 - Extended clipboard
		CheckMenuItem(m_hPopupMenuDisplay, ID_FULLSCREEN, MF_BYCOMMAND|MF_CHECKED);
		if (m_opts.m_ShowToolbar)
			SetWindowPos(m_hwndcn, m_hwndTBwin,0,m_TBr.bottom,m_winwidth, m_winheight, SWP_SHOWWINDOW);
		else  {
			SetWindowPos(m_hwndcn, m_hwndTBwin,0,0,cx, cy, SWP_SHOWWINDOW);
			SetWindowPos(m_hwndTBwin, NULL ,0,0,0, 0, SWP_HIDEWINDOW);
		}
		TitleBar.DisplayWindow(TRUE, TRUE); //Added by: Lars Werner (http://lars.werner.no)
 		if (m_opts.m_ViewOnly)TitleBar.SetText(m_desktopName_viewonly);
		else TitleBar.SetText(m_desktopName); //Added by: Lars Werner (http://lars.werner.no)

	} else {
		if (m_opts.m_ShowToolbar)
			SetWindowPos(m_hwndcn, m_hwndTBwin, 0, m_TBr.bottom, m_winwidth, m_winheight, SWP_SHOWWINDOW);
		else {
			SetWindowPos(m_hwndcn, m_hwndTBwin, 0, 0, m_winwidth, m_winheight, SWP_SHOWWINDOW);
			SetWindowPos(m_hwndTBwin, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
		}
		ShowWindow(m_hwndMain, SW_NORMAL);
		style = GetWindowLong(m_hwndMain, GWL_STYLE);
		style &= ~(WS_MAXIMIZE | WS_POPUP);
		style |= WS_DLGFRAME | WS_THICKFRAME | WS_CAPTION;
		SetWindowLong(m_hwndMain, GWL_STYLE, style);
		SetWindowPos(m_hwndMain, HWND_NOTOPMOST, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED |SWP_NOREDRAW); //Modified by: Lars Werner (http://lars.werner.no) - Reason: Bugfix, The framework got invisible after moving, so a NCCALCSIZE needed to be called!
		// adzm - 2010-07 - Extended clipboard
		CheckMenuItem(m_hPopupMenuDisplay, ID_FULLSCREEN, MF_BYCOMMAND|MF_UNCHECKED);
		TitleBar.DisplayWindow(FALSE, TRUE); //Added by: Lars Werner (http://lars.werner.no)
		if (m_hwndStatus)::RedrawWindow(m_hwndStatus, NULL,NULL, RDW_INVALIDATE); //Added by: Lars Werner (http://lars.werner.no) - Reason: The status window is not getting redrawn after a resize.
	}
}

void ClientConnection::BorderlessMode()
{
	ShowWindow(m_hwndMain, SW_NORMAL);
	LONG style = GetWindowLong(m_hwndMain, GWL_STYLE);
	style &= ~(WS_CAPTION |WS_DLGFRAME | WS_THICKFRAME);
	SetWindowLong(m_hwndMain, GWL_STYLE, style);
	m_opts.m_ShowToolbar = false;	
	SetWindowPos(m_hwndMain, HWND_NOTOPMOST, 0, 0, 100, 100, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED); 

	SetWindowPos(m_hwndcn, m_hwndTBwin, 0, 0, 100, 100, SWP_SHOWWINDOW);
	SetWindowPos(m_hwndTBwin, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);

	// adzm - 2010-07 - Extended clipboard
	CheckMenuItem(m_hPopupMenuDisplay, ID_FULLSCREEN, MF_BYCOMMAND | MF_UNCHECKED);
	TitleBar.DisplayWindow(FALSE, TRUE);
	if (m_hwndStatus)::RedrawWindow(m_hwndStatus, NULL, NULL, RDW_INVALIDATE);
}

bool ClientConnection::BumpScroll(int x, int y)
{
	int dx = 0;
	int dy = 0;
	int rightborder = GetSystemMetrics(SM_CXSCREEN)-BUMPSCROLLBORDER;
	int bottomborder = GetSystemMetrics(SM_CYSCREEN)-BUMPSCROLLBORDER-(m_TBr.bottom - m_TBr.top);
	if (x < BUMPSCROLLBORDER)
		dx = -BUMPSCROLLAMOUNTX * m_opts.m_scale_num / m_opts.m_scale_den;
	if (x >= rightborder)
		dx = +BUMPSCROLLAMOUNTX * m_opts.m_scale_num / m_opts.m_scale_den;;
	if (y < BUMPSCROLLBORDER)
		dy = -BUMPSCROLLAMOUNTY * m_opts.m_scale_num / m_opts.m_scale_den;;
	if (y >= bottomborder)
		dy = +BUMPSCROLLAMOUNTY * m_opts.m_scale_num / m_opts.m_scale_den;;
	if (dx || dy) {
		if (ScrollScreen(dx,dy)) {
			// If we haven't physically moved the cursor, artificially
			// generate another mouse event so we keep scrolling.
			POINT p;
			GetCursorPos(&p);
			if (p.x == x && p.y == y)
				SetCursorPos(x,y);
			return true;
		} 
	}
	return false;
}
