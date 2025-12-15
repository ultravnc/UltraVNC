// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "vncdesktopthread.h"
#include "Localization.h"
#include "vnctimedmsgbox.h"
namespace
{
    int g_Oldcounter=0;
}
bool stop_hookwatch=false;

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

DWORD WINAPI hookwatch(LPVOID lpParam)
{
	vncDesktopThread *dt = (vncDesktopThread *)lpParam;

	while (!stop_hookwatch)
	{
		if (dt->g_obIPC.listall()->counter!=g_Oldcounter)
		{
			dt->m_desktop->TriggerUpdate();
			Sleep(15);
		}
		Sleep(5);
	}

	return 0;
}

bool
vncDesktopThread::Handle_Ringbuffer(mystruct *ringbuffer,rfb::Region2D &rgncache)
{
	bool returnvalue=false;
	if (ringbuffer->counter==g_Oldcounter) return 0;
	int counter=ringbuffer->counter;
	if (counter<1 || counter>1999) return 0;

	if (g_Oldcounter<counter)
	{
		for (int i =g_Oldcounter+1; i<=counter;i++)
		{
			if (ringbuffer->type[i]==0)
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				rect.tl.x-=m_desktop->m_ScreenOffsetx;
				rect.br.x-=m_desktop->m_ScreenOffsetx;
				rect.tl.y-=m_desktop->m_ScreenOffsety;
				rect.br.y-=m_desktop->m_ScreenOffsety;		
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
#ifdef _DEBUG

					OutputDevMessage("SCHook changeRct %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);	
#endif
					returnvalue=true;
					rgncache.assign_union(rect);
				}
			}			
		}
	}
	else
	{
		int  i = 0;
		for (i =g_Oldcounter+1;i<2000;i++)
		{
			if (ringbuffer->type[i]==0 )
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				rect.tl.x-=m_desktop->m_ScreenOffsetx;
				rect.br.x-=m_desktop->m_ScreenOffsetx;
				rect.tl.y-=m_desktop->m_ScreenOffsety;
				rect.br.y-=m_desktop->m_ScreenOffsety;	
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
#ifdef _DEBUG

					OutputDevMessage("SCHook changeRct %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);	
#endif
					returnvalue=true;
					rgncache.assign_union(rect);
				}
			}			
		}
		for (i=1;i<=counter;i++)
		{
			if (ringbuffer->type[i]==0 )
			{
				rfb::Rect rect;
				rect.tl = rfb::Point(ringbuffer->rect1[i].left, ringbuffer->rect1[i].top);
				rect.br = rfb::Point(ringbuffer->rect1[i].right, ringbuffer->rect1[i].bottom);
				//Buffer coordinates
				rect.tl.x-=m_desktop->m_ScreenOffsetx;
				rect.br.x-=m_desktop->m_ScreenOffsetx;
				rect.tl.y-=m_desktop->m_ScreenOffsety;
				rect.br.y-=m_desktop->m_ScreenOffsety;
				
				rect = rect.intersect(m_desktop->m_Cliprect);
				if (!rect.is_empty())
				{
#ifdef _DEBUG

					OutputDevMessage("SCHook changeRct %i %i %i %i  \n",rect.tl.x,rect.br.x,rect.tl.y,rect.br.y);	
#endif
					returnvalue=true;
					rgncache.assign_union(rect);
				}
			}			
		}
	}
	g_Oldcounter=counter;
	return returnvalue;
}

