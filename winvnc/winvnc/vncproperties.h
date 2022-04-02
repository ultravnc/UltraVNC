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

class vncProperties;

#if (!defined(_WINVNC_VNCPROPERTIES))
#define _WINVNC_VNCPROPERTIES

// Includes
// Marscha@2004 - authSSP: objbase.h needed for CoInitialize etc.
#include <objbase.h>
#include "stdhdrs.h"
#include "vncserver.h"
#include "vncsetauth.h"
#include "inifile.h"
#include <userenv.h>
// The vncProperties class itself
class vncProperties
{
public:
	vncProperties();
	void LoadFromIniFile();
	void ShowAdmin();
	BOOL Init(vncServer* server);

private:
	
	static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SaveToIniFile();
	void ExpandBox(HWND hDlg, BOOL fExpand);
	void InitPortSettings(HWND hwnd);
	int			cx, cy;
	bool		m_pref_Lock_service_helper;
	BOOL		bExpanded;
	HBITMAP		hBmpExpand;
	HBITMAP		hBmpCollaps;
	vncServer* m_server;
	vncSetAuth	m_vncauth;
	BOOL		m_dlgvisible;
	IniFile		myIniFile;
};

#endif // _WINVNC_VNCPROPERTIES
