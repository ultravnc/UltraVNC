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
 



#pragma once
#include "VNCOptions.h"
#include "MRU.h"
#include <DSMPlugin/DSMPlugin.h>

class SessionDialog  
{
public:

	// Create a connection dialog, with the options to be
	// displayed if the options.. button is clicked.
	int SetQuickOption(SessionDialog* p_SD, HWND hwnd);
	int ManageQuickOptions(SessionDialog* _this, HWND hwnd);
	SessionDialog(VNCOptions *pOpt, ClientConnection* pCC, CDSMPlugin* pDSMPlugin); // sf@2002
	int DoDialog();
	int m_port;
	int m_proxyport;
	bool m_fUseProxy;
	bool m_fFromOptions; // sf@2002
	bool m_fFromFile; // sf@2002
	TCHAR m_host_dialog[256];
	TCHAR m_proxyhost[256];
//	TCHAR m_remotehost[256];
   	virtual ~SessionDialog();

	ClientConnection *m_pCC;
	VNCOptions *m_pOpt;
	MRU *m_pMRU;
	CDSMPlugin* m_pDSMPlugin; // sf@2002

private:
	//ClientConnection *m_pCC;
	//VNCOptions *m_pOpt;
	//MRU *m_pMRU;
	//CDSMPlugin* m_pDSMPlugin; // sf@2002
	//static BOOL CALLBACK SessDlgProc(  HWND hwndDlg,  UINT uMsg, 
	//	WPARAM wParam, LPARAM lParam );

};
