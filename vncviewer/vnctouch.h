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


#include "stdhdrs.h"

#ifdef _Gii
#ifndef VNCTOUCH_H__
#define VNCTOUCH_H__

#include "vncviewer.h"
#include "ClientConnection.h"

#define MI_WP_SIGNATURE 0xFF515700
#define SIGNATURE_MASK 0xFFFFFF00
#define IsPenEvent(dw) (((dw) & SIGNATURE_MASK) == MI_WP_SIGNATURE)
#define IsTouchEvent(dw) (((dw) & 0x80) == 0x80)


struct MyTouchINfo
{
	DWORD TouchId;
	DWORD pointerflag;
	DWORD touchmask;
	int X;
	int Y;	
	int ContactWidth;
	int ContactHeight;
};

class vnctouch
{
public:
	vnctouch();
	~vnctouch();	
	void OnTouch(HWND hWnd, WPARAM wParam, LPARAM lParam);
	int _handle_gii_message(HWND hwnd);
	void Set_ClientConnect(ClientConnection *IN_cc);
	bool TouchActivated(){return IsTouchActivated;};

private:
	void Activate_touch(HWND hWnd);
	int GetContactIndex(int dwID);
	int rfb_send_gii_mt_event();
	int scale_coordinates(int x, int y, int *nx, int *ny);
	int gettimeofday(struct timeval* p, void* tz /* IGNORED */);
	void rfb_gii_init_timestamps(DWORD *time_msec, DWORD64 *time_usec);
	void doczka_vertical();

	int _find_gii_version(uint16_t min, uint16_t max);
	int _handle_gii_device_creation(void);
	int _handle_gii_version_message(rfbGIIMsg *msg, int gii_bigendian);	
	void rfb_gii_init_valuator(rfbGIIValuatorEventMsg *val, int cnt);
	int  rfb_send_gii_mt_empty_event(rfbGIIValuatorEventMsg *val, DWORD time_msec, DWORD64 time_usec);
	
	void Initialize_Vars();
	void AddFlag(DWORD *flag, DWORD value);
	bool IsFlagSet(DWORD flag, DWORD value);

	MyTouchINfo *pMyTouchInfo;
	int *idLookup;
	bool *point_down;
	int MAXPOINTS;
	ClientConnection *cc;
	uint32_t gii_deviceOrigin;
	struct timeval start_time;
	bool IsTouchActivated;
	uint16_t serverVersion;
};
#endif
#endif