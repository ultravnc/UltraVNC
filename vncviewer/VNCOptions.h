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

#ifndef VNCOPTIONS_H__
#define VNCOPTIONS_H__

#pragma once

#include <vector>

#ifdef _XZ
#define LASTENCODING rfbEncodingZSTDYWRLE
#else
#define LASTENCODING rfbEncodingZSTDYWRLE
#endif
#define NOCURSOR 0
#define DOTCURSOR 1
#define NORMALCURSOR 2

class VNCOptions
{
public:
	VNCOptions();
	VNCOptions& operator=(VNCOptions& s);
	virtual ~VNCOptions();

	// Save and load a set of options from a config file
	void Save(char* fname);
	void Load(char* fname);

	// process options
	bool	m_listening;
	int     m_listenPort;
	bool	m_connectionSpecified;
	bool	m_configSpecified;
	TCHAR   m_cmdlnUser[256]; // act: add user option on command line
	TCHAR   m_clearPassword[256]; // Modif sf@2002
	int     m_quickoption; // Modif sf@2002 - v1.1.2
	TCHAR   m_configFilename[_MAX_PATH];
	bool	m_restricted;

	// default connection options - can be set through Dialog
	bool	m_ViewOnly;
	bool	m_NoStatus;
	bool	m_NoHotKeys;
	bool	m_FullScreen;
	bool	m_SavePos;
	bool	m_SaveSize;
	bool	m_Directx;
	bool    m_ShowToolbar;

	bool	autoDetect;
	int		m_Use8Bit;
	std::vector<int> m_PreferredEncodings;

	bool	m_SwapMouse;
	bool    m_Emul3Buttons;
	bool    m_JapKeyboard;
	int     m_Emul3Timeout;
	int     m_Emul3Fuzz;
	bool	m_Shared;
	bool    m_NoBorder;
	bool	m_DeiconifyOnBell;
	bool	m_DisableClipboard;
	int     m_localCursor;
	int     m_throttleMouse; // adzm 2010-10
	bool	m_scaling;
	bool    m_fAutoScaling;
	int		m_scale_num, m_scale_den; // Numerator & denominator
	// Tight specific
	bool	m_useCompressLevel;
	int		m_compressLevel;
	bool	m_enableJpegCompression;
	int		m_jpegQualityLevel;
	bool	m_requestShapeUpdates;
	bool	m_ignoreShapeUpdates;

	// Modif sf@2002 - Server Scaling
	int		m_nServerScale; // Divider of the Target Server's screensize
	int m_reconnectcounter;
	int m_x, m_y, m_w, m_h;
	bool    m_fEnableCache;
	bool    m_fEnableZstd;
	bool	m_fUseDSMPlugin;
	TCHAR   m_szDSMPluginFilename[_MAX_PATH];
	bool	m_oldplugin;
	int m_saved_scale_num;
	int m_saved_scale_den;
	bool m_saved_scaling;
	TCHAR	m_kbdname[9];
	bool	m_kbdSpecified;
	bool	m_UseEnc[LASTENCODING + 1];
	TCHAR   m_host_options[256];
	int     m_port;
	TCHAR   m_proxyhost[256];
	int     m_proxyport;
	bool	m_fUseProxy;
	bool	m_allowMonitorSpanning;
	bool	m_ChangeServerRes;
	bool	m_extendDisplay;
	bool	m_use_virt;
	bool	m_use_allmonitors;
	int		m_requestedWidth;
	int		m_requestedHeight;

	int     m_logLevel;
	bool    m_logToFile, m_logToConsole;
	TCHAR   m_logFilename[_MAX_PATH];
	int m_delay;
	int	m_autoReconnect;
	bool m_NoMoreCommandLineUserPassword;
	bool m_fExitCheck; //PGM @ Advantig
	int m_FTTimeout;
	int m_keepAliveInterval;
	int m_IdleInterval;
	bool m_fAutoAcceptIncoming;
	bool m_fAutoAcceptNoDSM;
	bool m_fRequireEncryption;
	bool m_preemptiveUpdates;
	void CheckProxyAndHost();
#ifdef _Gii
	bool m_giienable;
#endif

	int DoDialog(bool running = false, HWND hwnd = NULL);
	void SetFromCommandLine(LPTSTR szCmdLine);

	void CancelDialog();
	void setDefaultDocumentPath();
	static BOOL CALLBACK OptDlgProc(HWND hwndDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam);

	TCHAR m_document_folder[MAX_PATH];
	TCHAR m_prefix[56];
	TCHAR m_imageFormat[56];
	bool m_running;

	TCHAR m_optionfile[MAX_PATH];
	void VNCOptions::setDefaultOptionsFileName();
	TCHAR* VNCOptions::getDefaultOptionsFileName();

private:
	void ShowUsage(LPTSTR info = NULL);
	void FixScaling();

	HWND hwnd;
};

#endif VNCOPTIONS_H__