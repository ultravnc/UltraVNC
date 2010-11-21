/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////

typedef struct tagPATCH NEAR*    HPATCH;
#define WM_APP       0x8000


HPATCH WINAPI   PatchDdi(HWND, HGLOBAL, DDITYPE);

BOOL   WINAPI   UnpatchDdi(HPATCH); 
BOOL   WINAPI   SetTimeOut(BOOL);


//
// Messages to the hwnd passed in to PatchDdi
//

#define WM_DDICALL          (WM_APP)
    // wParam is type
    // lParam is LPSTR, allocated out of app's heap

#define WM_ENDDDIPATCH      (WM_APP+1)
    // wParam is nothing
    // lParam is count of DDI trapped calls



