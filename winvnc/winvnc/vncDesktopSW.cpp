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
	//vnclog.Print(LL_INTINFO, VNCLOG("GetSize \n"));
	if (!m_screenCapture) {
		m_SWOffsetx=0;
		m_SWOffsety=0;
		return rfb::Rect(0, 0, m_scrinfo.framebufferWidth, m_scrinfo.framebufferHeight);
	} else {
		if (show_multi_monitors) {
			int nWidth = mymonitor[0].Width;
			int nHeight = mymonitor[0].Height;				
			switch (nr_monitors) {
				case 2:
					nWidth = m_Cliprect.br.x;
					nHeight = m_Cliprect.br.y;
					break;
				case 3:
					{
						switch (m_current_monitor) {
							case 4:
							{
								if (nr_monitors > 2) {
									if ((mymonitor[0].offsetx < mymonitor[1].offsetx && mymonitor[1].offsetx < mymonitor[2].offsetx) ||
										(mymonitor[1].offsetx < mymonitor[0].offsetx && mymonitor[0].offsetx < mymonitor[2].offsetx))
										nWidth = mymonitor[0].Width+mymonitor[1].Width;
									if ((mymonitor[0].offsetx < mymonitor[2].offsetx && mymonitor[2].offsetx < mymonitor[1].offsetx) ||
										(mymonitor[2].offsetx < mymonitor[0].offsetx && mymonitor[0].offsetx < mymonitor[2].offsetx))
										nWidth = mymonitor[0].Width+mymonitor[2].Width;
									if ((mymonitor[1].offsetx < mymonitor[2].offsetx && mymonitor[2].offsetx < mymonitor[0].offsetx) ||
										(mymonitor[2].offsetx < mymonitor[1].offsetx && mymonitor[1].offsetx < mymonitor[0].offsetx))
										nWidth = mymonitor[1].Width+mymonitor[2].Width;
									} else
										nWidth = mymonitor[3].Width;
									nHeight = max(mymonitor[0].Height, mymonitor[1].Height);
							} break;
							case 5:
							{
								if (nr_monitors > 2) {
									if ((mymonitor[0].offsetx < mymonitor[1].offsetx && mymonitor[1].offsetx < mymonitor[2].offsetx) ||
										(mymonitor[0].offsetx < mymonitor[2].offsetx && mymonitor[2].offsetx < mymonitor[1].offsetx))
										nWidth = mymonitor[1].Width+mymonitor[2].Width;
									if ((mymonitor[1].offsetx < mymonitor[0].offsetx && mymonitor[0].offsetx < mymonitor[2].offsetx) ||
										(mymonitor[1].offsetx < mymonitor[2].offsetx && mymonitor[2].offsetx < mymonitor[0].offsetx))
										nWidth = mymonitor[0].Width+mymonitor[2].Width;
									if ((mymonitor[2].offsetx < mymonitor[1].offsetx && mymonitor[1].offsetx < mymonitor[0].offsetx) ||
										(mymonitor[2].offsetx < mymonitor[0].offsetx && mymonitor[0].offsetx < mymonitor[1].offsetx))
										nWidth = mymonitor[0].Width+mymonitor[1].Width;
								} else
									nWidth = mymonitor[3].Width;
									nHeight = max(mymonitor[1].Height, mymonitor[2].Height);
							} break;
							default:
							{
								nWidth = mymonitor[3].Width;
								nHeight = mymonitor[3].Height;
							}
						}
					}
					default: break;
				}
				return rfb::Rect(0,0,nWidth,nHeight);
			} else
				return rfb::Rect(0,0,mymonitor[m_current_monitor-1].Width,mymonitor[m_current_monitor-1].Height);
		}
}

void
vncDesktop::SetBitmapRectOffsetAndClipRect(int offesetx, int offsety, int width, int height)
{
	if (width == 0)
		width = mymonitor[3].Width;
	if (height == 0)
		height = mymonitor[3].Height;

	m_ScreenOffsetx = mymonitor[3].offsetx;
	m_ScreenOffsety = mymonitor[3].offsety;
	if (!show_multi_monitors) {
		m_ScreenOffsetx = mymonitor[0].offsetx;
		m_ScreenOffsety = mymonitor[0].offsety;
	}
	m_bmrect = rfb::Rect(offesetx, offsety, width, height);
	m_SWOffsetx = m_bmrect.tl.x;
	m_SWOffsety = m_bmrect.tl.y;
	m_Cliprect.tl.x = 0;
	m_Cliprect.tl.y = 0;
	m_Cliprect.br.x = m_bmrect.br.x;
	m_Cliprect.br.y = m_bmrect.br.y;
	if (m_screenCapture && (m_current_monitor == MULTI_MON_PRIMARY || m_current_monitor == MULTI_MON_SECOND || m_current_monitor == MULTI_MON_THIRD)) {
		int mon = m_current_monitor -1;
		m_Cliprect.tl.x = mymonitor[mon].offsetx;
		m_Cliprect.tl.y = mymonitor[mon].offsety;
		m_Cliprect.br.x = mymonitor[mon].offsetx + mymonitor[mon].Width;
		m_Cliprect.br.y = mymonitor[mon].offsety + mymonitor[mon].Height;
	}
}

