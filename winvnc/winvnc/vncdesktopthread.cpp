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

#include "vncdesktopthread.h"
#include "vncOSVersion.h"
#include "uvncUiAccess.h"
extern bool G_USE_PIXEL;

bool g_DesktopThread_running;
DWORD WINAPI hookwatch(LPVOID lpParam);
extern bool stop_hookwatch;
void testBench();
char g_hookstring[16]="";
bool PreConnect = false;

inline bool
ClipRect(int *x, int *y, int *w, int *h,
	    int cx, int cy, int cw, int ch) {
  if (*x < cx) {
    *w -= (cx-*x);
    *x = cx;
  }
  if (*y < cy) {
    *h -= (cy-*y);
    *y = cy;
  }
  if (*x+*w > cx+cw) {
    *w = (cx+cw)-*x;
  }
  if (*y+*h > cy+ch) {
    *h = (cy+ch)-*y;
  }
  return (*w>0) && (*h>0);
}

////////////////////////////////////////////////////////////////////////////////////
// Modif rdv@2002 - v1.1.x - videodriver
void
vncDesktopThread::copy_bitmaps_to_buffer(ULONG i,rfb::Region2D &rgncache,rfb::UpdateTracker &tracker)
{
	
		rfb::Rect rect;
		int x = m_desktop->pchanges_buf->pointrect[i].rect.left;
		int w = m_desktop->pchanges_buf->pointrect[i].rect.right-m_desktop->pchanges_buf->pointrect[i].rect.left;
		int y = m_desktop->pchanges_buf->pointrect[i].rect.top;
		int h = m_desktop->pchanges_buf->pointrect[i].rect.bottom-m_desktop->pchanges_buf->pointrect[i].rect.top;
		//vnclog.Print(LL_INTINFO, VNCLOG("Driver ************* %i %i %i %i \n"),x,y,w,h);

		if (!ClipRect(&x, &y, &w, &h, 0,0,m_desktop->m_bmrect.br.x, m_desktop->m_bmrect.br.y)) return;
#ifdef _DEBUG
		static DWORD sLastCopy = GetTickCount();
		DWORD now = GetTickCount();
#if 1
		OutputDevMessage("%4d REct1 %i %i %i %i", now - sLastCopy, x, y, w, h);
#else
					char			szText[256];
		sprintf_s(szText, "%4d REct1 %i %i %i %i  \n", now - sLastCopy, x, y, w, h);
					OutputDebugString(szText);		
#endif
		sLastCopy = now;
#endif
		rect.tl.x = x;
		rect.br.x = x+w;
		rect.tl.y = y;
		rect.br.y = y+h;

		switch(m_desktop->pchanges_buf->pointrect[i].type)
			{
				case SCREEN_SCREEN:
					{
						int dx=m_desktop->pchanges_buf->pointrect[i].point.x;
						int dy=m_desktop->pchanges_buf->pointrect[i].point.y;
						if (!m_screen_moved && (dx==0 || dy==0) )
								{
//// Fix in case !Cliprect
									int xx=x;
                                    int yy=y;
                                    int hh=h;
                                    int ww=w;
                                    if (ClipRect(&xx,&yy,&ww,&hh,0,0, m_desktop->m_bmrect.br.x, m_desktop->m_bmrect.br.y))
									{
                                    rect.tl.x=xx;
                                    rect.tl.y=yy;
                                    rect.br.x=xx+ww;
                                    rect.br.y=yy+hh;
                                    rgncache.assign_union(rect);
									}

//////////////////////
// Fix Eckerd
									x=x+dx;;
									y=y+dy;;
									if (!ClipRect(&x,&y,&w,&h,0,0,m_desktop->m_bmrect.br.x, m_desktop->m_bmrect.br.y)) return;
//////////////////////
// Fix Eckerd
									rect.tl.x=x-dx;
									rect.tl.y=y-dy;
									rect.br.x=x+w-dx;
									rect.br.y=y+h-dy;

									rfb::Point delta = rfb::Point(-dx,-dy);
									rgncache.assign_union(rect);
									tracker.add_copied(rect, delta);
								//	vnclog.Print(LL_INTINFO, VNCLOG("Copyrect \n"));
								}
						else
								{
									rgncache.assign_union(rect);
								}
						break;
					}

				case SOLIDFILL:
				case TEXTOUT:
				case BLEND:
				case TRANS:
				case PLG:
				case BLIT:;
					rgncache.assign_union(rect);
					break;
				case POINTERCHANGE:
					break;
				default:
					break;
			}
}



// Modif rdv@2002 - v1.1.x - videodriver
BOOL
vncDesktopThread::handle_driver_changes(rfb::Region2D &rgncache,rfb::UpdateTracker &tracker)
{ 

	omni_mutex_lock l(m_desktop->m_screenCapture_lock,70);

	if (!m_desktop->m_screenCapture)
		return FALSE;
	int	oldaantal = m_desktop->m_screenCapture->getPreviousCounter();
	int counter=m_desktop->pchanges_buf->counter;
//	int nr_updates=m_desktop->pchanges_buf->pointrect[0].type;
//	vnclog.Print(LL_INTERR, VNCLOG("updates, rects %i\n"),oldaantal-counter);
	if (oldaantal==counter) return FALSE;
	if (counter<1 || counter >1999) return FALSE;
//	m_desktop->pchanges_buf->pointrect[0].type=0;
	m_screen_moved=m_desktop->CalcCopyRects(tracker);

	if (oldaantal<counter)
		{
			for (int i =oldaantal+1; i<=counter;i++)
				{
					copy_bitmaps_to_buffer(i,rgncache,tracker);
				}

		}
	else
		{
		    int i = 0;
			for (i =oldaantal+1;i<MAXCHANGES_BUF;i++)
				{
					copy_bitmaps_to_buffer(i,rgncache,tracker);
				}
			for (i=1;i<=counter;i++)
				{
					copy_bitmaps_to_buffer(i,rgncache,tracker);
				}
		}	
//	vnclog.Print(LL_INTINFO, VNCLOG("Nr rects %i \n"),rgncache.Numrects());
	if (m_desktop->m_screenCapture)
		m_desktop->m_screenCapture->setPreviousCounter(counter);
// A lot updates left after combining 
// This generates an overflow
// We expand each single update to minimum 32x32
	if (rgncache.Numrects()>150)
	{
		rfb::Region2D rgntemp;
		rfb::RectVector rects;
		rfb::RectVector::iterator i;
		rgncache.get_rects(rects, 1, 1);
		for (i = rects.begin(); i != rects.end(); i++)
			{
				rfb::Rect rect = *i;
				rect.tl.x=rect.tl.x/32*32;
				rect.tl.y=rect.tl.y/32*32;
				rect.br.x=rect.br.x/32*32+32;
				rect.br.y=rect.br.y/32*32+32;
				if (rect.br.x>m_desktop->m_bmrect.br.x) rect.br.x=m_desktop->m_bmrect.br.x;
				if (rect.br.y>m_desktop->m_bmrect.br.y) rect.br.y=m_desktop->m_bmrect.br.y;
				rgntemp.assign_union(rect);
			}
//Still to many little updates
//Use the bounding rectangle for updates
		if (rgntemp.Numrects()>50)
		{
			Rect brect=rgntemp.get_bounding_rect();
			rgncache.clear();
			rgncache.assign_union(brect);
		}
		else
		{
		rgncache.clear();
		rgncache.assign_union(rgntemp);
		}
	}
	return TRUE;
}

BOOL
vncDesktopThread::Init(vncDesktop *desktop, vncServer *server)
{
	// Save the server pointer
	framecounter = 0;
	m_server = server;
	m_desktop = desktop;
	m_returnset = FALSE;
	m_returnsig = new omni_condition(&m_returnLock);
	// Start the thread
	start_undetached();
	// Wait for the thread to let us know if it failed to init
	{	omni_mutex_lock l(m_returnLock,71);

		while (!m_returnset)
		{
			m_returnsig->wait();
		}
	}
	if (m_return!=0) 
    {
        g_DesktopThread_running=false;
    }
	return m_return;
}

void
vncDesktopThread::ReturnVal(DWORD result)
{
	omni_mutex_lock l(m_returnLock,72);
	m_returnset = TRUE;
	m_return = result;
	m_returnsig->signal();
}

void
vncDesktopThread::PollWindow(rfb::Region2D &rgn, HWND hwnd)
{
	// Are we set to low-load polling?
	if (m_server->PollOnEventOnly())
	{
		// Yes, so only poll if the remote user has done something
		if (!m_server->RemoteEventReceived()) {
			return;
		}
	}

	// Does the client want us to poll only console windows?
	if (m_desktop->m_server->PollConsoleOnly())
	{
		char classname[20];

		// Yes, so check that this is a console window...
		if (GetClassName(hwnd, classname, sizeof(classname))) {
			if ((strcmp(classname, "tty") != 0) &&
				(strcmp(classname, "ConsoleWindowClass") != 0)) {
				return;
			}
		}
	}

	RECT rect;

	// Get the rectangle
	if (GetWindowRect(hwnd, &rect)) {
		//Buffer coordinates
			rect.left-=m_desktop->m_ScreenOffsetx;
			rect.right-=m_desktop->m_ScreenOffsetx;
			rect.top-=m_desktop->m_ScreenOffsety;
			rect.bottom-=m_desktop->m_ScreenOffsety;
		rfb::Rect wrect = rfb::Rect(rect).intersect(m_desktop->m_Cliprect);
		if (!wrect.is_empty()) {
			rgn.assign_union(wrect);
		}
	}
}
static int old_inputDesktopSelected;
bool vncDesktopThread::handle_display_change(HANDLE& threadHandle, rfb::Region2D& rgncache, rfb::SimpleUpdateTracker& clipped_updates, rfb::ClippedUpdateTracker& updates)
{
	int inputDesktopSelected = vncService::InputDesktopSelected();
	if (inputDesktopSelected == 2) {
		m_desktop->m_buffer.WriteMessageOnScreen("UltraVVNC running as application doesn't \nhave permission to acces \nUAC protected windows.\n\nScreen is locked until the remote user \nunlock this window");
		rfb::Rect rect;
		rect.tl = rfb::Point(0,0);
		rect.br = rfb::Point(300,120);
		rgncache.assign_union(rect);
	}
	else if (inputDesktopSelected == 1) {
		if (old_inputDesktopSelected == 2)
			m_desktop->m_displaychanged = true;
	}
	old_inputDesktopSelected = inputDesktopSelected;

    //vnclog.Print(LL_INTINFO, VNCLOG("desktop MM %i IsMM %i CurMon %d\n"), m_desktop->requested_all_monitor, m_desktop->m_buffer.IsAllMonitors(), m_desktop->m_current_monitor);
		
	if (m_desktop->m_displaychanged ||									
			vncService::InputDesktopSelected()==0 ||							//handle logon and screensaver desktops
			m_desktop->m_hookswitch||										//hook change request
			m_desktop->requested_all_monitor!=m_desktop->m_buffer.IsAllMonitors() ||		//monitor change request
			m_desktop->m_old_monitor != m_desktop->m_current_monitor
			){
		// We need to wait until viewer has send if he support Size changes
		if (!m_server->All_clients_initialalized()) {
					Sleep(30);
					vnclog.Print(LL_INTERR, VNCLOG("Wait for viewer init \n"));
		}
		//logging
		if (m_desktop->m_displaychanged)								
			vnclog.Print(LL_INTERR, VNCLOG("++++Screensize changed \n"));
		if (m_desktop->m_hookswitch)									
			vnclog.Print(LL_INTERR, VNCLOG("m_hookswitch \n"));
		if (m_desktop->requested_all_monitor!=m_desktop->m_buffer.IsAllMonitors()) 
			vnclog.Print(LL_INTERR, VNCLOG("desktop switch %i %i \n"),m_desktop->requested_all_monitor,m_desktop->m_buffer.IsAllMonitors());
		if (!m_server->IsThereFileTransBusy())
			if (vncService::InputDesktopSelected()==0)						
				vnclog.Print(LL_INTERR, VNCLOG("++++InputDesktopSelected \n"));
				
		BOOL monitor_changed=false;
		rfbServerInitMsg oldscrinfo;
		//*******************************************************
		// Lock Buffers from here
		//*******************************************************
		{
			if (XRichCursorEnabled) m_server->UpdateCursorShape();
			/// We lock all buffers,,and also back the client thread update mechanism
			omni_mutex_lock l(m_desktop->m_update_lock,273);
			// We remove all queue updates from the tracker
			m_server->Clear_Update_Tracker();
			// Also clear the current updates
			rgncache.clear();
			// Also clear the copy_rect updates
			clipped_updates.clear();
			// TESTTESTTEST
			// Are all updates cleared....old updates could generate bounding errors
			// any other queues to clear ? Yep cursor positions
			m_desktop->m_cursorpos.tl.x=0;
			m_desktop->m_cursorpos.tl.y=0;
			m_desktop->m_cursorpos.br.x=0;
			m_desktop->m_cursorpos.br.y=0;
			//keep a copy of the old screen size, so we can check for changes later on
			oldscrinfo = m_desktop->m_scrinfo;
						
			if (m_desktop->requested_all_monitor!=m_desktop->m_buffer.IsAllMonitors()) {
				m_desktop->Checkmonitors();
				m_desktop->requested_all_monitor=m_desktop->m_buffer.IsAllMonitors();

				bool orig_monitor=m_desktop->show_all_monitors;	
				m_desktop->show_all_monitors = true;
				if (m_desktop->requested_all_monitor && m_desktop->nr_monitors>1) 
					m_desktop->show_all_monitors = true;
				else 
					m_desktop->show_all_monitors = false;							
				if ( orig_monitor!=m_desktop->show_all_monitors) {
						monitor_changed = true;
						m_desktop->m_old_monitor = m_desktop->m_current_monitor;
				}
			}
			// JnZn558
			else {
				if (m_desktop->m_old_monitor != m_desktop->m_current_monitor) {
					if (m_desktop->m_old_monitor != MULTI_MON_ALL)
						monitor_changed = true;
					m_desktop->m_old_monitor = m_desktop->m_current_monitor;
				}
			}

				//*******************************************************
				// Reinitialize buffers,color, etc
				// monitor change, for non driver, use another buffer
				//*******************************************************
				if (!m_server->IsThereFileTransBusy())
					if (m_desktop->m_displaychanged || vncService::InputDesktopSelected()==0 || m_desktop->m_hookswitch || (monitor_changed && !m_desktop->m_screenCapture)) {
						// Attempt to close the old hooks
						// shutdown(true) driver is reinstalled without shutdown,(shutdown need a 640x480x8 switch)
						vnclog.Print(LL_INTERR, VNCLOG("m_desktop->Shutdown"));
						monitor_changed=false;
						if (!m_desktop->Shutdown()) {
							vnclog.Print(LL_INTERR, VNCLOG("Shutdown KillAuthClients\n"));
							m_server->KillAuthClients();
							return false;
						}					
						bool fHookDriverWanted = (FALSE != m_desktop->m_hookdriver);
                        Sleep(1000);
						vnclog.Print(LL_INTERR, VNCLOG("m_desktop->Startup"));
						if (m_desktop->Startup() != 0) {
							vnclog.Print(LL_INTERR, VNCLOG("Startup KillAuthClients\n"));
							m_server->KillAuthClients();
							SetEvent(m_desktop->restart_event);
							return false;
						}

						if (m_desktop->m_screenCapture && !XRichCursorEnabled)
								m_desktop->m_screenCapture->hardwareCursor();

						if (m_desktop->m_screenCapture && XRichCursorEnabled) 
								m_desktop->m_screenCapture->noHardwareCursor();

						m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx, m_desktop->m_ScreenOffsety, m_desktop->nr_monitors == 1);

						// sf@2003 - After a new Startup(), we check if the required video driver
						// is actually available. If not, we force hookdll
						// No need for m_hookswitch again because the driver is NOT available anyway.
						// All the following cases are now handled:
						// 1. Desktop thread starts with "Video Driver" checked and no video driver available...
						//    -> HookDll forced (handled by the first InitHookSettings() after initial Startup() call
						// 2. Desktop Thread starts without "Video Driver" checked but available driver
						//    then the user checks "Video Driver" -> Video Driver used
						// 3. Desktop thread starts with "Video Driver" and available driver used
						//    Then driver is switched off (-> hookDll) 
						//    Then the driver is switched on again (-> hook driver used again)
						// 4. Desktop thread starts without "Video Driver" checked and no driver available
						//    then the users checks "Video Driver" 
						if (fHookDriverWanted && m_desktop->m_screenCapture == NULL) {
							vnclog.Print(LL_INTERR, VNCLOG("m_videodriver == NULL \n"));
							m_desktop->SethookMechanism(false,false); 	// InitHookSettings() would work as well;
						}
						stop_hookwatch=true;
						vnclog.Print(LL_INTERR, VNCLOG("threadHandle \n"));
						if (threadHandle) {
							WaitForSingleObject( threadHandle, INFINITE );
							CloseHandle(threadHandle);
							stop_hookwatch=false;
							threadHandle=NULL;
						}
						vnclog.Print(LL_INTERR, VNCLOG("threadHandle2 \n"));

					}
					//*******************************************************
					// end reinit
					//*******************************************************

					if ((m_desktop->m_scrinfo.framebufferWidth != oldscrinfo.framebufferWidth) ||
							(m_desktop->m_scrinfo.framebufferHeight != oldscrinfo.framebufferHeight)) {
						m_desktop->m_screensize_changed = true;
						vnclog.Print(LL_INTINFO, VNCLOG("SCR: new screen format %dx%dx%d\n"),
								m_desktop->m_scrinfo.framebufferWidth,
								m_desktop->m_scrinfo.framebufferHeight,
								m_desktop->m_scrinfo.format.bitsPerPixel);
					}

					m_desktop->m_displaychanged = FALSE;
					m_desktop->m_hookswitch = FALSE;
					m_desktop->Hookdll_Changed = m_desktop->On_Off_hookdll; // Set the hooks again if necessary !
					//****************************************************************************
					//************* SCREEN SIZE CHANGED 
					//****************************************************************************

					if (m_desktop->m_screensize_changed) {
							vnclog.Print(LL_INTERR, VNCLOG("Size changed\n"));
							POINT CursorPos;
							GetCursorPos(&CursorPos);
							CursorPos.x -= m_desktop->m_ScreenOffsetx;
							CursorPos.y -= m_desktop->m_ScreenOffsety;
							m_desktop->m_cursorpos.tl = CursorPos;
							m_desktop->m_cursorpos.br = rfb::Point(GetSystemMetrics(SM_CXCURSOR),
							GetSystemMetrics(SM_CYCURSOR)).translate(CursorPos);
							m_server->SetBufferOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);
							// Adjust the UpdateTracker clip region
							updates.set_clip_region(m_desktop->m_Cliprect);
							m_desktop->m_buffer.ClearCache();
                            monitor_changed = true;
						}

					// JnZn558
					RECT rc = { 0 };
					//
					if (monitor_changed && m_desktop->m_screenCapture) {
						// we are using the driver, so a monitor change is a view change, like a special kind of single window
						// m_desktop->current_monitor is the new monitor we want to see
						// m_SWOffset is used by the encoders to send the correct coordinates to the viewer
						// Cliprect, buffer coordinates
						if (m_desktop->show_all_monitors) {
							vnclog.Print(LL_INTINFO, VNCLOG("Request Monitor %d\n"), m_desktop->m_current_monitor);
							//int mon[2] = {0};
							switch (m_desktop->m_current_monitor) {							
								case MULTI_MON_ALL:
								default:
								{
									m_desktop->m_SWOffsetx=0;
									m_desktop->m_SWOffsety=0;
									m_server->SetBufferOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);
									m_desktop->m_Cliprect.tl.x = 0;
									m_desktop->m_Cliprect.tl.y = 0;
									m_desktop->m_Cliprect.br.x = m_desktop->mymonitor[MULTI_MON_ALL].Width;
									m_desktop->m_Cliprect.br.y = m_desktop->mymonitor[MULTI_MON_ALL].Height;
									rc.right = m_desktop->mymonitor[MULTI_MON_ALL].Width;
									rc.bottom = m_desktop->mymonitor[MULTI_MON_ALL].Height;
									vnclog.Print(LL_INTINFO, VNCLOG("Monitor %d: width = %d height = %d\n"), m_desktop->m_current_monitor, rc.right, rc.bottom);


                                    m_desktop->m_ScreenOffsetx = m_desktop->mymonitor[MULTI_MON_ALL].offsetx;
                                    m_desktop->m_ScreenOffsety = m_desktop->mymonitor[MULTI_MON_ALL].offsety;
                                    m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx, m_desktop->m_ScreenOffsety, m_desktop->nr_monitors == 1);
								}
							}
						}
						else
						{							
							int nCurrentMon = m_desktop->m_current_monitor;
								
							m_desktop->m_SWOffsetx=m_desktop->mymonitor[nCurrentMon].offsetx-m_desktop->mymonitor[MULTI_MON_ALL].offsetx;
							m_desktop->m_SWOffsety=m_desktop->mymonitor[nCurrentMon].offsety-m_desktop->mymonitor[MULTI_MON_ALL].offsety;
							m_server->SetBufferOffset(m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety);

							m_desktop->m_ScreenOffsetx = m_desktop->mymonitor[nCurrentMon].offsetx;
							m_desktop->m_ScreenOffsety = m_desktop->mymonitor[nCurrentMon].offsety;
							m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx, m_desktop->m_ScreenOffsety, m_desktop->nr_monitors == 1);

							m_desktop->m_Cliprect.tl.x = m_desktop->mymonitor[nCurrentMon].offsetx - m_desktop->mymonitor[MULTI_MON_ALL].offsetx;
							m_desktop->m_Cliprect.tl.y = m_desktop->mymonitor[nCurrentMon].offsety - m_desktop->mymonitor[MULTI_MON_ALL].offsety;
							m_desktop->m_Cliprect.br.x = m_desktop->mymonitor[nCurrentMon].offsetx + m_desktop->mymonitor[nCurrentMon].Width - m_desktop->mymonitor[MULTI_MON_ALL].offsetx;
							m_desktop->m_Cliprect.br.y = m_desktop->mymonitor[nCurrentMon].offsety + m_desktop->mymonitor[nCurrentMon].Height - m_desktop->mymonitor[MULTI_MON_ALL].offsety;

							
							rc.right = m_desktop->mymonitor[nCurrentMon].Width;
							rc.bottom = m_desktop->mymonitor[nCurrentMon].Height;
							vnclog.Print(LL_INTINFO, VNCLOG("Monitor %d: width = %d height = %d\n"), m_desktop->m_current_monitor, rc.right, rc.bottom);
						}



						vnclog.Print(LL_INTERR, VNCLOG("***********###############************ %i %i %i %i %i %i\n"),m_desktop->m_SWOffsetx,m_desktop->m_SWOffsety
							,m_desktop->m_Cliprect.tl.x,m_desktop->m_Cliprect.tl.y,m_desktop->m_Cliprect.br.x,m_desktop->m_Cliprect.br.y);


						rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
						updates.set_clip_region(m_desktop->m_Cliprect);				
						m_desktop->m_buffer.ClearCache();
						m_desktop->m_buffer.BlackBack();


					}
					InvalidateRect(NULL,NULL,TRUE);
					rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));
					
					if (memcmp(&m_desktop->m_scrinfo.format, &oldscrinfo.format, sizeof(rfbPixelFormat)) != 0)
						{
							vnclog.Print(LL_INTERR, VNCLOG("Format changed\n"));
							m_server->UpdatePalette(false); // changed no lock ok
							//UpdateLocalFormat without updatelock can cause stuck in m_signal->wait(), because not returning from mutex->lock()
							//the synchonisation of EnableUpdates(TRUE|FALSE) does not work without getting the UpdateLock.
							//this is a weakness in the vnc server implementation
							//we had the problem on XP, running in a virtual machine of win7 virtualbox.
							m_server->UpdateLocalFormat(true); // must have the update lock
						}

					if (m_desktop->m_screensize_changed) 
						{
							m_desktop->m_screensize_changed = false;
							monitor_changed = false;
							m_server->SetNewSWSize(m_desktop->m_scrinfo.framebufferWidth,m_desktop->m_scrinfo.framebufferHeight, monitor_changed);//changed no lock ok
							m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx, m_desktop->m_ScreenOffsety, m_desktop->nr_monitors == 1);// no lock ok							
						}
					
					if (monitor_changed && m_desktop->m_screenCapture)
						{
							monitor_changed=false;
							m_server->SetNewSWSize(rc.right,rc.bottom,TRUE);
						}
			}// end lock
	}

	return true;
}

void vncDesktopThread::do_polling(HANDLE& threadHandle, rfb::Region2D& rgncache, int& fullpollcounter, bool cursormoved)
{
	// POLL PROBLEM AREAS
	// We add specific areas of the screen to the region cache,
	// causing them to be fetched for processing.
	// if can_be_hooked==false, hooking is temp disabled, use polling
	if (m_desktop->On_Off_hookdll) {
		if (m_desktop->SetHook && g_obIPC.listall()!=NULL && m_desktop->can_be_hooked) {
			strcpy_s(g_hookstring,"schook");
			DWORD dwTId(0);
			if (threadHandle==NULL) threadHandle = CreateThread(NULL, 0, hookwatch, this, 0, &dwTId);
			if (Handle_Ringbuffer(g_obIPC.listall(),rgncache)) return;
		}
		if (m_desktop->can_be_hooked && !m_desktop->m_hookinited && m_desktop->m_bitmappointer) {
			strcpy_s(g_hookstring, "");
			m_desktop->m_bitmappointer=false;
			m_desktop->m_DIBbits=NULL;
		}
	}
	

	DWORD lTime = GetTimeFunction();
	m_desktop->m_buffer.SetAccuracy(m_desktop->m_server->TurboMode() ? 8 : 4); 
	if (cursormoved)  {
		m_desktop->idle_counter=0;
		m_lLastMouseMoveTime = lTime;
	}

	if ((m_desktop->m_server->PollFullScreen()) || (!m_desktop->can_be_hooked && !cursormoved)) {
		int timeSinceLastMouseMove = lTime - m_lLastMouseMoveTime;			
		if (timeSinceLastMouseMove > 50) { // 50 ms pause after a Mouse move 
			++fullpollcounter;
			rfb::Rect r = m_desktop->GetSize();
			// THIS FUNCTION IS A PIG. It uses too much CPU on older machines (PIII, P4)
			if (vncService::InputDesktopSelected()!=2) {
				if (m_desktop->FastDetectChanges(rgncache, r, 0, true)) 
					capture=false;
			}
			else
				capture=false;
			// force full screen scan every three seconds after the mouse stops moving
			if (fullpollcounter > 20) {
				rgncache.assign_union(m_desktop->m_Cliprect);
				fullpollcounter = 0;
			}
		}
	}
		
    HWND hWndToPoll = 0;
	if (m_desktop->m_server->PollForeground() || !m_desktop->can_be_hooked) {
		// Get the window rectangle for the currently selected window
		hWndToPoll = GetForegroundWindow();
		if (hWndToPoll != NULL)
			 PollWindow(rgncache, hWndToPoll);
		
	}
	
	if (m_desktop->m_server->PollUnderCursor() || !m_desktop->can_be_hooked) {
		// Find the mouse position
		POINT mousepos;
		if (GetCursorPos(&mousepos)) {
			// Find the window under the mouse
			HWND hwnd = WindowFromPoint(mousepos);
            // exclude the foreground window (done above) and desktop
			if (hwnd != NULL && hwnd != hWndToPoll && hwnd != GetDesktopWindow())
				 PollWindow(rgncache, hwnd);

		}
	}
}


DWORD WINAPI ThreadCheckMirrorDriverUpdates(LPVOID lpParam)
{
	vncDesktopThread *dt = (vncDesktopThread *)lpParam;
	while (dt->looping)
	{	
			if  (dt->m_desktop->m_screenCapture && dt->m_desktop->m_screenCapture->getPreviousCounter() != dt->m_desktop->pchanges_buf->counter)
			{
				strcpy_s(g_hookstring,"driver");
				SetEvent(dt->m_desktop->trigger_events[0]);				
			}
		Sleep(5);
	}
	return 0;
}

DWORD WINAPI ThreadCheckCursorUpdates(LPVOID lpParam)
{
	vncDesktopThread *dt = (vncDesktopThread *)lpParam;
	POINT cursorpos;
	while (dt->looping)
	{
		if (GetCursorPos(&cursorpos) &&  ((cursorpos.x != dt->oldcursorpos.x) || (cursorpos.y != dt->oldcursorpos.y)))
				SetEvent(dt->m_desktop->trigger_events[0]);		
		Sleep(50);
	}
	return 0;
}



void *
vncDesktopThread::run_undetached(void *arg)
{		
	//*******************************************************
	// INIT
	//*******************************************************
	if (m_server->AutoCapt() == 1) {
		if (VNCOS.OS_VISTA||VNCOS.OS_WIN7||VNCOS.OS_WIN8||VNCOS.OS_WIN10) 
			G_USE_PIXEL=false;
		else 
			G_USE_PIXEL=true;//testBench();
	}
	else if (m_server->AutoCapt() == 2)
		G_USE_PIXEL=true;
	else
		G_USE_PIXEL=false;


	
	capture=true;
	vnclog.Print(LL_INTERR, VNCLOG("Hook changed 1\n"));
	// Save the thread's "home" desktop, under NT (no effect under 9x)
	m_desktop->m_home_desktop = GetThreadDesktop(GetCurrentThreadId());
    vnclog.Print(LL_INTERR, VNCLOG("Hook changed 2\n"));
	// Attempt to initialise and return success or failure
	m_desktop->KillScreenSaver();
	keybd_uni_event(VK_CONTROL, 0, 0, 0);
    keybd_uni_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
	Sleep(500); //Give screen some time to kill screensaver
    DWORD startup_error;
	if ((startup_error = m_desktop->Startup()) != 0) {
		vncService::SelectHDESK(m_desktop->m_home_desktop);
		if (m_desktop->m_input_desktop)
			CloseDesktop(m_desktop->m_input_desktop);
		ReturnVal(startup_error);
		return NULL;
	}
	// Succeeded to initialise ok
	ReturnVal(0);	

	// sf@2003 - Done here to take into account if the driver is actually activated
	m_desktop->InitHookSettings(); 
	initialupdate=false;	

	// All changes in the state of the display are stored in a local
	// UpdateTracker object, and are flushed to the vncServer whenever
	// client updates are about to be triggered
	rfb::SimpleUpdateTracker clipped_updates;
	rfb::ClippedUpdateTracker updates(clipped_updates, m_desktop->m_Cliprect);
	clipped_updates.enable_copyrect(true);
	rfb::Region2D rgncache;


	// Incoming update messages are collated into a single region cache
	// The region cache areas are checked for changes before an update
	// is triggered, and the changed areas are passed to the UpdateTracker
	rgncache = m_desktop->m_Cliprect;
	m_server->SetScreenOffset(m_desktop->m_ScreenOffsetx,m_desktop->m_ScreenOffsety,m_desktop->nr_monitors == 1);
	

	// The previous cursor position is stored, to allow us to erase the
	// old instance whenever it moves.	
	POINT tempcursorpos;
	GetCursorPos(&tempcursorpos);
	oldcursorpos = rfb::Point(tempcursorpos);
	// The driver gives smaller rectangles to check
	// if Accuracy is 4 you eliminate pointer updates
	if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver)
	{
		m_server->SetBufferOffset(m_desktop->m_SWOffsetx, m_desktop->m_SWOffsety);
		m_desktop->m_buffer.SetAccuracy(4);
	}

	//init vars
	m_desktop->Hookdll_Changed = true;
	m_desktop->m_displaychanged=false;
	m_desktop->m_hookswitch=false;
	m_desktop->m_hookinited = FALSE;
	m_desktop->m_bitmappointer = FALSE;

	int esc_counter = 0;
	while (!m_server->All_clients_initialalized())
	{
	Sleep(100);
	esc_counter++;
	if (esc_counter > 50) break;
	vnclog.Print(LL_INTERR, VNCLOG("Wait for viewer init \n"));
	}

	// Set driver cursor state
	XRichCursorEnabled= (FALSE != m_desktop->m_server->IsXRichCursorEnabled());
	if (!XRichCursorEnabled && m_desktop->m_screenCapture) m_desktop->m_screenCapture->hardwareCursor();
	if (XRichCursorEnabled && m_desktop->m_screenCapture) m_desktop->m_screenCapture->noHardwareCursor();
	if (XRichCursorEnabled) m_server->UpdateCursorShape();

	InvalidateRect(NULL,NULL,TRUE);
	oldtick=GetTimeFunction();
	oldtick2=GetTimeFunction();
	int fullpollcounter=0;
	//*******************************************************
	// END INIT
	//*******************************************************
	// START PROCESSING DESKTOP MESSAGES
	/////////////////////
	HANDLE threadHandle=NULL;
	stop_hookwatch=false;
	/////////////////////
	// We use a dynmiac value based on cpu usage
    //DWORD MIN_UPDATE_INTERVAL=33;
	/////////////////////
	looping=true;
	SetEvent(m_desktop->restart_event);
	
	rgncache.assign_union(rfb::Region2D(m_desktop->m_Cliprect));

	if (!PreConnect) {
		if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver && !VNCOS.OS_WIN8 && !VNCOS.OS_WIN10)
		{
			m_desktop->m_buffer.GrabRegion(rgncache,true,true);
		}
		else if (!VNCOS.OS_WIN8 && !VNCOS.OS_WIN10)
		{
			m_desktop->m_buffer.GrabRegion(rgncache,false,true);
		}
	}
	
	int waittime=0;

	// We set a flag inside the desktop handler here, to indicate it's now safe
	// to handle clipboard messages
	m_desktop->SetClipboardActive(TRUE);

	HANDLE ThreadHandleCheckMirrorDriverUpdates = NULL;
	HANDLE ThreadHandleCheckCursorUpdates = NULL;
	m_desktop->PreventScreensaver(true);
	
	if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver && !VNCOS.OS_WIN8 && !VNCOS.OS_WIN10)
		{
			//MIRROR DRIVER....still to check if this works			
			DWORD dw;
			if (ThreadHandleCheckMirrorDriverUpdates == NULL)
				ThreadHandleCheckMirrorDriverUpdates = CreateThread(NULL, 0, ThreadCheckMirrorDriverUpdates, this, 0, &dw);
			DWORD dw2;
			if (ThreadHandleCheckCursorUpdates == NULL)
				ThreadHandleCheckCursorUpdates = CreateThread(NULL, 0, ThreadCheckCursorUpdates, this, 0, &dw2);
			waittime = 1000;
		}
	else if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver && (VNCOS.OS_WIN8||VNCOS.OS_WIN10))
		{
			//DDENGINE
			m_desktop->trigger_events[6] = m_desktop->m_screenCapture->getHScreenEvent();
			m_desktop->trigger_events[7] = m_desktop->m_screenCapture->getHPointerEvent();			
			waittime = 1000;
			DWORD dw;
			if (XRichCursorEnabled && m_desktop->m_screenCapture && ThreadHandleCheckCursorUpdates == NULL)
				ThreadHandleCheckCursorUpdates = CreateThread(NULL, 0, ThreadCheckCursorUpdates, this, 0, &dw);

		}
	else {
			// BLIT
			DWORD dw;
			if (ThreadHandleCheckCursorUpdates == NULL)
				ThreadHandleCheckCursorUpdates = CreateThread(NULL, 0, ThreadCheckCursorUpdates, this, 0, &dw);
			waittime = 33; // possible this need to be higher to lower cpu usage
		}
			   
	while (looping && !fShutdownOrdered)
	{		
		DWORD result;
	
		result=WaitForMultipleObjects(8, m_desktop->trigger_events, FALSE, waittime);
		{
			
			// We need to wait until restart is done
			// else wait_timeout goes in to looping while sink window is not ready
			// if no window could be started in 10 seconds something went wrong, close
			// desktop thread.
			DWORD status=WaitForSingleObject(m_desktop->restart_event,10000);
			if (status==WAIT_TIMEOUT) 
				looping = false;
			switch(result)
			{
				case WAIT_FAILED:
					looping = false;
				break;
				case WAIT_OBJECT_0+6:
					ResetEvent(m_desktop->trigger_events[6]);
					strcpy_s(g_hookstring,"ddengine");
				case WAIT_TIMEOUT:				
				case WAIT_OBJECT_0: {
					ResetEvent(m_desktop->trigger_events[0]);
							{
#ifdef _DEBUG
						static DWORD sLastCopy3 = GetTickCount();
						DWORD now = GetTickCount();
						OutputDevMessage("WaitForMultipleObjects result . %d last call delta %4d ms", result, now - sLastCopy3);
						sLastCopy3 = now;
#endif
								// MaxCpu() == 100  PowerMode
								if (m_server->MaxCpu() != 100) {
									if ((fullpollcounter==10 || fullpollcounter==0 || fullpollcounter==5)) {
										cpuUsage = usage.GetUsage();
										if (cpuUsage > m_server->MaxCpu()) 
											MIN_UPDATE_INTERVAL+=10;
										else MIN_UPDATE_INTERVAL-=10;
										if (MIN_UPDATE_INTERVAL<MIN_UPDATE_INTERVAL_MIN) 
											MIN_UPDATE_INTERVAL=MIN_UPDATE_INTERVAL_MIN;
										if (MIN_UPDATE_INTERVAL>MIN_UPDATE_INTERVAL_MAX) 
											MIN_UPDATE_INTERVAL=MIN_UPDATE_INTERVAL_MAX;
									}
								}
								else
									MIN_UPDATE_INTERVAL = 25;

								// MAX 30fps
								newtick = GetTimeFunction(); 
								if ((newtick-oldtick)<MIN_UPDATE_INTERVAL)
									Sleep(MIN_UPDATE_INTERVAL-(newtick-oldtick));
								
								if (m_desktop->VideoBuffer() && m_desktop->m_hookdriver) 
									handle_driver_changes(rgncache,updates);								

								//*******************************************************
								// HOOKDLL START STOP need to be executed from the thread
								//*******************************************************
								if (m_desktop->Hookdll_Changed && !m_desktop->m_hookswitch) {
									vnclog.Print(LL_INTERR, VNCLOG("Hook changed \n"));
									m_desktop->StartStophookdll(m_desktop->On_Off_hookdll);
									if (m_desktop->On_Off_hookdll)
										m_desktop->m_hOldcursor = NULL; // Force mouse cursor grabbing if hookdll On
									// Todo: in case of hookdriver Off - Hoodll On -> hookdriver On - Hoodll Off
									// we must send an empty mouse cursor to the clients so they get rid of their local
									// mouse cursor bitmap
									m_desktop->Hookdll_Changed=false;
								}
								//*******************************************************
								// SCREEN DISPLAY HAS CHANGED, RESTART DRIVER (IF Used)
								//*******************************************************
								if (!m_server->IsThereFileTransBusy() && (!handle_display_change(threadHandle, rgncache, clipped_updates, updates))) {
									//failed we need to quit thread
									looping = false;
									break;
								}
								//*******************************************************
								// END SCREEN DISPLAY HAS CHANGED
								//*******************************************************
							
								////////////////////////////////////////////////////////////////////////////////
								// END DYNAMIC CHANGES
								////////////////////////////////////////////////////////////////////////////////

								// CALCULATE CHANGES
								m_desktop->m_UltraEncoder_used = m_desktop->m_server->IsThereAUltraEncodingClient();
								m_desktop->m_Ultra2Encoder_used = m_desktop->m_server->IsThereAUltra2EncodingClient();

								omni_mutex_lock l(m_desktop->m_update_lock, 275);
								if (m_desktop->m_server->UpdateWanted() || !initialupdate) {
									//omni_mutex_lock l(m_desktop->m_update_lock, 275);
									oldtick=newtick;
									bool cursormoved = false;
									POINT cursorpos;
									if (GetCursorPos(&cursorpos) && ((cursorpos.x != oldcursorpos.x) ||(cursorpos.y != oldcursorpos.y))) {
										cursormoved = TRUE;
										oldcursorpos = rfb::Point(cursorpos);
										m_desktop->m_server->UpdateMouse();
										if (MyGetCursorInfo) {
											MyCURSORINFO cinfo;
											cinfo.cbSize=sizeof(MyCURSORINFO);
											MyGetCursorInfo(&cinfo);
											m_desktop->m_hcursor = cinfo.hCursor;
										}
									}

									//****************************************************************************
									//************* Check for moved windows
									//****************************************************************************
									// Back removed, to many artifacts
									bool s_moved=false;
									if (!m_desktop->m_hookdriver) 
											s_moved=m_desktop->CalcCopyRects(updates);

								
									//****************************************************************************
									//************* Polling ---- no driver
									//****************************************************************************
									if (!m_desktop->m_hookdriver || !m_desktop->can_be_hooked || m_desktop->no_default_desktop) {
										if (!s_moved)
											do_polling(threadHandle, rgncache, fullpollcounter, cursormoved);
									}
									//****************************************************************************
									//************* driver  No polling
									//****************************************************************************
									else  {
										if (cursormoved)
											m_desktop->m_buffer.SetAccuracy(m_desktop->m_server->TurboMode() ? 2 : 1);
										else
											m_desktop->m_buffer.SetAccuracy(m_desktop->m_server->TurboMode() ? 4 : 2); 
									}
									
									
									// PROCESS THE MOUSE POINTER
									// Some of the hard work is done in clients, some here
									// This code fetches the desktop under the old pointer position
									// but the client is responsible for actually encoding and sending
									// it when required.
									// This code also renders the pointer and saves the rendered position
									// Clients include this when rendering updates.
									// The code is complicated in this way because we wish to avoid 
									// rendering parts of the screen the mouse moved through between
									// client updates, since in practice they will probably not have changed.
								
									if (cursormoved && !m_desktop->m_hookdriver && !m_desktop->m_cursorpos.is_empty()) {
										// Cursor position seems to be outsite the bounding
										// When you make the screen smaller
										// add extra check
										rfb::Rect rect;
										int x = m_desktop->m_cursorpos.tl.x;
										int w = m_desktop->m_cursorpos.br.x-x;
										int y = m_desktop->m_cursorpos.tl.y;
										int h = m_desktop->m_cursorpos.br.y-y;
										if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
												m_desktop->m_bmrect.br.x+m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y+m_desktop->m_bmrect.tl.y)) {
											rect.tl.x = x;
											rect.br.x = x+w;
											rect.tl.y = y;
											rect.br.y = y+h;
											rgncache.assign_union(rect);
										}
									}
									

									{
										// Prevent any clients from accessing the Buffer
										omni_mutex_lock ll(m_desktop->m_update_lock,276);
										
										// CHECK FOR COPYRECTS
										// This actually just checks where the Foreground window is
										// Back added, no need to stop polling during move
										if ((cpuUsage < m_server->MaxCpu()/2) && !m_desktop->m_hookdriver && !s_moved)
											s_moved=m_desktop->CalcCopyRects(updates);
										
										// GRAB THE DISPLAY
										// Fetch data from the display to our display cache.
										// Update the scaled rects when using server side scaling
										// something wrong inithooking again
										// We make sure no updates are in the regions
										// sf@2002 - Added "&& m_desktop->m_hookdriver"
										// Otherwise we're still getting driver updates (from shared memory buffer)
										// after a m_hookdriver switching from on to off 
										// (and m_hookdll from off to on) that causes mouse cursor garbage,
										// or missing mouse cursor.		
										if (PreConnect && m_desktop->m_server->IsEncoderSet())
											m_desktop->m_buffer.WriteMessageOnScreenPreConnect();
										else if (!PreConnect && m_desktop->VideoBuffer() && m_desktop->m_hookdriver ) {
											m_desktop->m_buffer.GrabRegion(rgncache, true, capture);
										}
										else if (!PreConnect)
											m_desktop->m_buffer.GrabRegion(rgncache,false,capture);

										capture=true;
											
										// sf@2002 - v1.1.x - Mouse handling
										// If one client, send cursor shapes only when the cursor changes.
										// This is Disabled for now.
										if( !XRichCursorEnabled==m_desktop->m_server->IsXRichCursorEnabled()) {
											XRichCursorEnabled= (FALSE != m_desktop->m_server->IsXRichCursorEnabled());
											if (m_desktop->m_screenCapture)
												if (!XRichCursorEnabled) 
													m_desktop->m_screenCapture->hardwareCursor();
												else 
													m_desktop->m_screenCapture->noHardwareCursor();
										}

										if (m_desktop->m_server->IsXRichCursorEnabled()) {
											if (m_desktop->m_hcursor != m_desktop->m_hOldcursor || m_desktop->m_buffer.IsShapeCleared()) {
												m_desktop->m_hOldcursor = m_desktop->m_hcursor;
												m_desktop->m_buffer.SetCursorPending(TRUE);
												if (!m_desktop->m_hookdriver  || !m_desktop->can_be_hooked) 
													m_desktop->m_buffer.GrabMouse(); // Grab mouse cursor in all cases
												m_desktop->m_server->UpdateMouse();
												rfb::Rect rect;
												int x = m_desktop->m_cursorpos.tl.x;
												int w = m_desktop->m_cursorpos.br.x-x;
												int y = m_desktop->m_cursorpos.tl.y;
												int h = m_desktop->m_cursorpos.br.y-y;
												if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
												m_desktop->m_bmrect.br.x+m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y+m_desktop->m_bmrect.tl.y)) {
													rect.tl.x = x;
													rect.br.x = x+w;
													rect.tl.y = y;
													rect.br.y = y+h;
													rgncache.assign_union(rect);
												}
												m_server->UpdateCursorShape();
											}
										}
										else if (!m_desktop->m_hookdriver  || !m_desktop->can_be_hooked) {												
											// Render the mouse
											//if (!m_desktop->VideoBuffer())
											m_desktop->m_buffer.GrabMouse();											
											if (cursormoved) {
												// Inform clients that it has moved
												m_desktop->m_server->UpdateMouse();
												// Get the buffer to fetch the pointer bitmap
												if (!m_desktop->m_cursorpos.is_empty())	{
													rfb::Rect rect;
													int x = m_desktop->m_cursorpos.tl.x;
													int w = m_desktop->m_cursorpos.br.x-x;
													int y = m_desktop->m_cursorpos.tl.y;
													int h = m_desktop->m_cursorpos.br.y-y;
													if (ClipRect(&x, &y, &w, &h, m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.tl.y,
															m_desktop->m_bmrect.br.x+m_desktop->m_bmrect.tl.x, m_desktop->m_bmrect.br.y+m_desktop->m_bmrect.tl.y)) {
														rect.tl.x = x;
														rect.br.x = x+w;
														rect.tl.y = y;
														rect.br.y = y+h;
														rgncache.assign_union(rect);
													}
												}
											}
										}	
										
											
										// SCAN THE CHANGED REGION FOR ACTUAL CHANGES
										// The hooks return hints as to areas that may have changed.
										// We check the suggested areas, and just send the ones that
										// have actually changed.
										// Note that we deliberately don't check the copyrect destination
										// here, to reduce the overhead & the likelihood of corrupting the
										// backbuffer contents.
										rfb::Region2D checkrgn;
										rfb::Region2D changedrgn;
										rfb::Region2D cachedrgn;
											
										//Update the backbuffer for the copyrect region
										if (!clipped_updates.get_copied_region().is_empty()) {
											rfb::UpdateInfo update_info;
											rfb::RectVector::const_iterator i;
											clipped_updates.get_update(update_info);
											if (!update_info.copied.empty()) 
												for (i=update_info.copied.begin(); i!=update_info.copied.end(); i++) 						
													m_desktop->m_buffer.CopyRect(*i, update_info.copy_delta);
										}
										//Remove the copyrect region from the other updates																
										checkrgn = rgncache.subtract(clipped_updates.get_copied_region());	
										//make sure the copyrect is checked next update
										if (!clipped_updates.get_copied_region().is_empty() && (cpuUsage < m_server->MaxCpu()/2)) {

											rfb::UpdateInfo update_info;
											rfb::RectVector::const_iterator i;
											clipped_updates.get_update(update_info);
											if (!update_info.copied.empty())  {
												for (i=update_info.copied.begin(); i!=update_info.copied.end(); i++) {
													rfb::Rect rect;
													rect.br.x=i->br.x+4;
													rect.br.y=i->br.y+4;
													rect.tl.x=i->tl.x-4;
													rect.tl.y=i->tl.y-4;
													if (m_desktop->m_screenCapture)
														rect = rect.intersect(m_desktop->m_Cliprect);
													rgncache=rgncache.union_(rect);
													rfb::Rect src = rect.translate(update_info.copy_delta.negate());
													src = src.intersect(m_desktop->m_Cliprect);
													rgncache=rgncache.union_(src);
												}
											}
										}
										else
											rgncache = clipped_updates.get_copied_region();

										
										//Check all regions for changed and cached parts
										//This is very cpu intensive, only check once for all viewers
										//checkrgn = checkrgn.intersect(m_desktop->m_Cliprect);

										if (!checkrgn.is_empty()) {
											if (m_desktop->m_screenCapture)
												m_desktop->m_screenCapture->Lock();
											bool fullframe = false;
											if (m_desktop->m_Ultra2Encoder_used) {
												framecounter++;
												if (framecounter > 10) {
													framecounter = 0;
													checkrgn.assign_union(m_desktop->m_Cliprect);
													fullframe = true;
												}
											}

											m_desktop->m_buffer.CheckRegion(changedrgn,cachedrgn, checkrgn, fullframe);

											if(m_desktop->m_screenCapture)
												m_desktop->m_screenCapture->Unlock();
										}

										m_server->initialCapture_done();
										if (!initialupdate) {
											m_server->InitialUpdate(true);
											initialupdate=true;
											m_desktop->m_old_monitor = m_desktop->m_old_monitor == MULTI_MON_ALL ? 1 : MULTI_MON_ALL;
										}
										updates.add_changed(changedrgn);
										updates.add_cached(cachedrgn);
												
										clipped_updates.get_update(m_server->GetUpdateTracker());
									}  // end mutex lock

									// Clear the update tracker and region cache an solid
									clipped_updates.clear();
									// screen blanking
									if (m_desktop->m_screen_in_powersave && (!VNCOS.CaptureAlphaBlending() || m_desktop->VideoBuffer())) {
										DWORD new_timer=GetTickCount();
										if ((new_timer-monitor_sleep_timer)>500) {
											SendMessage(m_desktop->m_hwnd,WM_SYSCOMMAND,SC_MONITORPOWER,(LPARAM)2);
											monitor_sleep_timer=new_timer;
										}
									}

					#ifdef AVILOG
									if (m_desktop->AviGen) m_desktop->AviGen->AddFrame((BYTE*)m_desktop->m_DIBbits);
					#endif
								}
							}
						}
					break;

				case WAIT_OBJECT_0+1:
					ResetEvent(m_desktop->trigger_events[1]);
					m_desktop->lock_region_add=true;
					rgncache.assign_union(m_desktop->rgnpump);
					m_desktop->rgnpump.clear();
					m_desktop->lock_region_add=false;
					break;
				case WAIT_OBJECT_0+2:
					ResetEvent(m_desktop->trigger_events[2]);
					break;
				case WAIT_OBJECT_0+7:
					ResetEvent(m_desktop->trigger_events[7]);
				case WAIT_OBJECT_0+3:
					if (MyGetCursorInfo)
					{
						MyCURSORINFO cinfo;
						cinfo.cbSize=sizeof(MyCURSORINFO);
						MyGetCursorInfo(&cinfo);
						m_desktop->m_hcursor = cinfo.hCursor;
					}
					ResetEvent(m_desktop->trigger_events[3]);
					break;
				case WAIT_OBJECT_0+4:
					m_desktop->lock_region_add = true;
					rgncache.assign_union(m_desktop->m_Cliprect);
					ResetEvent(m_desktop->trigger_events[4]);
					m_desktop->lock_region_add = false;
					break;
				case WAIT_OBJECT_0+5:
					//break to close
					looping = false;
					ResetEvent(m_desktop->trigger_events[5]);
					break;
			}
		}
		
	}//while
	looping = false;
	m_server->KillAuthClients();

	stop_hookwatch=true;
	if (threadHandle)
	{
		WaitForSingleObject( threadHandle, 5000 );
		CloseHandle(threadHandle);
		threadHandle = NULL;
	}
	if (ThreadHandleCheckMirrorDriverUpdates)
	{
		WaitForSingleObject( ThreadHandleCheckMirrorDriverUpdates, 5000 );
		CloseHandle(ThreadHandleCheckMirrorDriverUpdates);
		ThreadHandleCheckMirrorDriverUpdates = NULL;
	}
	if (ThreadHandleCheckCursorUpdates)
	{
		WaitForSingleObject( ThreadHandleCheckCursorUpdates, 5000 );
		CloseHandle(ThreadHandleCheckCursorUpdates);
		ThreadHandleCheckCursorUpdates = NULL;
	}
	
	m_desktop->SetClipboardActive(FALSE);
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread\n"));
	
	// Clear all the hooks and close windows, etc.
    m_desktop->SetBlockInputState(false);
	m_desktop->PreventScreensaver(false);
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:SetBlockInputState\n"));
	
	// Clear the shift modifier keys, now that there are no remote clients
	vncKeymap::ClearShiftKeys();
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:ClearShiftKeys\n"));
	
	// Switch back into our home desktop, under NT (no effect under 9x)
	//TAG14
	HWND mywin=FindWindow("blackscreen",NULL);
	if (mywin)SendMessage(mywin,WM_CLOSE, 0, 0);
	g_DesktopThread_running=false;
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:g_DesktopThread_running=false\n"));
	m_desktop->Shutdown();
	vnclog.Print(LL_INTINFO, VNCLOG("quitting desktop server thread:m_desktop->Shutdown\n"));
	return NULL;
}

void writeLog(char *sender, char *text)
{
#ifdef _DEBUG
	char mess[3000];
	SYSTEMTIME st;
	GetSystemTime(&st);       // get current time
	int len = sprintf_s(mess, "%04d-%02d-%02d %02d:%02d:%02d.%03d %40s %s\r\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, sender, text);

	OutputDebugString(mess);
#endif
}

void WriteLog(char* sender, char *format, ...)
{
	char temp[1000];
	va_list args;
	va_start(args, format);
	vsprintf_s(temp, format, args);
	va_end(args);
	writeLog(sender, temp);
}
