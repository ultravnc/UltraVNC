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


//===========================================================================
//	FullScreen Titlebar Constants
//	2004 - All rights reservered
//===========================================================================
//
//	Project/Product :	FullScreenTitlebar
//  FileName		:	FullScreenTitleBarConst.h
//	Author(s)		:	Lars Werner
//  Homepage		:	http://lars.werner.no
//
//	Description		:	All the consts used in the FullScreentitlebar
//                  
//	Classes			:	None
//
//	History
//	Vers.  Date      Aut.  Type     Description
//  -----  --------  ----  -------  -----------------------------------------
//	1.00   22 01 04  LW    Create   Original
//===========================================================================

//Width and heigth for the toolbar
#define ctbWidth			700
#define ctbHeigth			20

//Default size on every picture used as buttons
#define ctbcxPicture		16
#define ctbcyPicture		14

//Margins for placing buttons on screen
#define ctbTopSpace			3
#define ctbLeftSpace		20
#define ctbRightSpace		20
#define ctbButtonSpace		1

//Color and layout
#define tbFont				"Arial"
#define tbFontSize			10
#define tbTextColor			RGB(220,220,220)
#define tbStartColor		RGB(64,64,64)
#define tbEndColor			RGB(32,32,32)
#define tbGradientWay		FALSE	//TRUE = Vertical, FALSE = Horiz
#define tbBorderPenColor	RGB(192,192,192)
#define tbBorderPenShadow	RGB(100,100,100)

//Triangularpoint is used to make the RGN so the window is not rectangular
#define tbTriangularPoint	10

//This is the width of the pen used to draw the border...
#define tbBorderWidth		2

//About showing the window
//adzm 2009-06-21 - If we are waiting on a repeater connection, and tbHideAtStartup is FALSE, then
//there will be a thin white line (and a hung window) while it waits to connect! TODO - this should
//only apply (i think) if we are starting up in full screen.
#define tbHideAtStartup		TRUE //Hide window when created
#define tbPinNotPushedIn	TRUE //Is the pin pushed in or out at startup (sorry for invertion!)
#define tbScrollWindow		TRUE //Animate window to scroll up/down
#define tbScrollDelay		20	//Timer variable for scrolling the window (cycletime) [ms]
#define tbAutoScrollTime	10	//* tbAutoScrollDelay milliseconds steps. Meaning if it is 10 then = 10 (steps) * 100ms (tbAutoScrollDelay) = 1000ms delay
#define tbScrollTimerID		1	//Timer id
#define tbAutoScrollTimer	2	//Timer id
#define tbAutoScrollDelay	100 //Timer variable for how many times the cursor is not over the window. If it is tbAutoScrollTime then it will hide if autohide

//=================================================
//Resource part
//=================================================

#define tbIDC_CLOSE			10
#define tbIDC_MAXIMIZE		20
#define tbIDC_MINIMIZE		30
#define tbIDC_PIN			40

#define tbIDC_SCREEN		50
#define tbIDC_PHOTO			60
#define tbIDC_SWITCHMONITOR	70
#define tbIDC_CHAT			80
#define tbIDC_FT			90

//=================================================
// Windows Message part
//=================================================

//FALSE = Send a custon WM message, TRUE = Send Minimize, maximize and close to parent
#define tbDefault			FALSE

//Own defines messages
#define tbWM_CLOSE			WM_USER+1000
#define tbWM_MINIMIZE		WM_USER+1001
#define tbWM_MAXIMIZE		WM_USER+1002

#define tbWM_FITSCREEN		WM_USER+1003
#define tbWM_NOSCALE		WM_USER+1004
#define tbWM_PHOTO			WM_USER+1005
#define tbWM_SWITCHMONITOR	WM_USER+1006
#define tbWM_CHAT			WM_USER+1007
#define tbWM_FT				WM_USER+1008

//=================================================
// Menus with ID's and messages
//=================================================

#define tbMENUID			IDR_tbMENU //Resource name for the menu
#define tbWMCOMMANDIDStart	50000 //Start: This is the internal id number sendt into the WM_COMMAND on each item
#define tbWMCOMMANDIDEnd	51000 //End: This is the internal id number sendt into the WM_COMMAND on each item
#define tbWMUSERID			2000 //This is WM_USER+n setting. Eg: if first item is clicked you will get an WM_USER+n+0 to the parent, and WM_USER+n+1 for the next item ect ect
#define tbLastIsStandard	TRUE //TRUE = Bottom of the menu is close, minimize and maximize, FALSE = The menu does not contain anything
