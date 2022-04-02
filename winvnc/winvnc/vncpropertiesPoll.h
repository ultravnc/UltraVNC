/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2010 UltraVNC Team Members. All Rights Reserved.
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
class vncPropertiesPoll;

#if (!defined(_WINVNC_VNCPROPERTIESPOLL))
#define _WINVNC_VNCPROPERTIESPOLL

// Includes
#include "stdhdrs.h"
#include "vncserver.h"
#include "inifile.h"
#include <userenv.h>
// The vncPropertiesPoll class itself
class vncPropertiesPoll
{
public:
	// Constructor/destructor
	vncPropertiesPoll();
	~vncPropertiesPoll();
	BOOL Init(vncServer *server);

	// The dialog box window proc
	static BOOL CALLBACK DialogProcPoll(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Show();

	// Ini file
	IniFile myIniFile;
	void LoadFromIniFile();
	void SaveToIniFile();

	// Implementation
protected:
	// The server object to which this properties object is attached.
	vncServer *			m_server;
	BOOL m_dlgvisible;
};

#endif // _WINVNC_vncPropertiesPoll
