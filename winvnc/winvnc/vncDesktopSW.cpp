/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


// System headers
#include <assert.h>
#include "stdhdrs.h"

// Custom headers
#include <omnithread.h>
#include "winvnc.h"
#include "vnchooks/VNCHooks.h"
#include "vncserver.h"
#include "rfbRegion.h"
#include "rfbRect.h"
#include "vncdesktop.h"
#include "vncservice.h"

rfb::Rect vncDesktop::GetSize()
{
	if (!m_screenCapture) {
		m_SWOffsetx=0;
		m_SWOffsety=0;
		return rfb::Rect(0, 0, m_scrinfo.framebufferWidth, m_scrinfo.framebufferHeight);
	} else {
		if (show_all_monitors) {
			int nWidth = mymonitor[MULTI_MON_PRIMARY].Width;
			int nHeight = mymonitor[MULTI_MON_PRIMARY].Height;				
			switch (nr_monitors) {
				case 2:
					nWidth = m_Cliprect.br.x;
					nHeight = m_Cliprect.br.y;
					break;
				case 3:
					{
						nWidth = mymonitor[MULTI_MON_ALL].Width;
						nHeight = mymonitor[MULTI_MON_ALL].Height;
					}
					default: break;
				}
				return rfb::Rect(0,0,nWidth,nHeight);
		} 
		else
			return rfb::Rect(0,0,mymonitor[m_current_monitor].Width,mymonitor[m_current_monitor].Height);
	}
}

void
vncDesktop::SetBitmapRectOffsetAndClipRect(int offesetx, int offsety, int width, int height)
{
	if (width == 0)
		width = mymonitor[MULTI_MON_ALL].Width;
	if (height == 0)
		height = mymonitor[MULTI_MON_ALL].Height;

	if (show_all_monitors) {
		m_ScreenOffsetx = mymonitor[MULTI_MON_ALL].offsetx;
		m_ScreenOffsety = mymonitor[MULTI_MON_ALL].offsety;
	}
	else {
		m_ScreenOffsetx = mymonitor[MULTI_MON_PRIMARY].offsetx;
		m_ScreenOffsety = mymonitor[MULTI_MON_PRIMARY].offsety;
	}
	m_bmrect = rfb::Rect(offesetx, offsety, width, height);
	m_SWOffsetx = m_bmrect.tl.x;
	m_SWOffsety = m_bmrect.tl.y;
	m_Cliprect.tl.x = 0;
	m_Cliprect.tl.y = 0;
	m_Cliprect.br.x = m_bmrect.br.x;
	m_Cliprect.br.y = m_bmrect.br.y;
	if (m_screenCapture && m_current_monitor != MULTI_MON_ALL) {
		m_SWOffsetx = mymonitor[m_current_monitor].offsetx - mymonitor[MULTI_MON_ALL].offsetx;
		m_SWOffsety = mymonitor[m_current_monitor].offsety - mymonitor[MULTI_MON_ALL].offsety;

		m_ScreenOffsetx = mymonitor[m_current_monitor].offsetx;
		m_ScreenOffsety = mymonitor[m_current_monitor].offsety;

		m_Cliprect.tl.x = mymonitor[m_current_monitor].offsetx - mymonitor[MULTI_MON_ALL].offsetx;
		m_Cliprect.tl.y = mymonitor[m_current_monitor].offsety - mymonitor[MULTI_MON_ALL].offsety;
		m_Cliprect.br.x = mymonitor[m_current_monitor].offsetx + mymonitor[m_current_monitor].Width - mymonitor[MULTI_MON_ALL].offsetx;
		m_Cliprect.br.y = mymonitor[m_current_monitor].offsety + mymonitor[m_current_monitor].Height - mymonitor[MULTI_MON_ALL].offsety;
	}
}

