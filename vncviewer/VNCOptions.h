// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#ifndef VNCOPTIONS_H__
#define VNCOPTIONS_H__

#define MAX_HOST_NAME_LEN 250
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

// Connection type enumeration
enum ConnectionType {
	DIRECT_TCP = 0,      // Direct TCP connection
	REPEATER_SERVER = 1, // Repeater server (proxy mode)
	UDP_BRIDGE = 2       // Bridge over UDP NAT
};

inline bool SwitchMatch(LPCTSTR arg, LPCTSTR swtch) {
	return (arg[0] == '-' || arg[0] == '/') &&
		(_tcsicmp(&arg[1], swtch) == 0);
}

class VNCOptions
{
public:
	VNCOptions();
	VNCOptions& operator=(VNCOptions& s);
	virtual ~VNCOptions();

	// Save and load a set of options from a config file
	void SaveOptions(char* fname);
	void LoadOptions(char* fname);

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
	bool	m_ipv6;
	bool m_AllowUntrustedServers;

	// default connection options - can be set through Dialog
	bool	m_ViewOnly;
	bool	m_NoStatus;
	bool	m_NoHotKeys;
	bool	m_FullScreen;
	TCHAR	m_language[32];  // Language preference (en, fr, de, es)
	bool	m_SavePos;
	bool	m_SaveSize;
	bool	m_Directx;
	bool    m_ShowToolbar;
	bool	m_GNOME;

	bool	autoDetect;
	int		m_Use8Bit;
	std::vector<int> m_PreferredEncodings;

	bool	m_SwapMouse;
	bool	m_BlockSameMouse;
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
	bool    m_fAutoScalingEven;
	bool    m_fAutoScalingLimit;
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
	TCHAR   m_host_options[MAX_HOST_NAME_LEN];
	int     m_port;
	TCHAR   m_proxyhost[MAX_HOST_NAME_LEN];
	int     m_proxyport;
	ConnectionType	m_connectionType;
	bool	m_allowMonitorSpanning;
	bool	m_ChangeServerRes;
	bool	m_extendDisplay;
	bool	m_showExtend;
	bool	m_use_virt;
	bool	m_useAllMonitors;
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
	bool m_UseOnlyDefaultConfigFile;
	bool m_preemptiveUpdates;
	void CheckProxyAndHost();
#ifdef _Gii
	bool m_giiEnable;
#endif

	int DoDialog(bool running = false, HWND hwnd = NULL);
	void SetFromCommandLine(LPTSTR szCmdLine);
	char szCmdLine[8191]{};

	void CancelDialog();
	void setDefaultDocumentPath();
	static BOOL CALLBACK OptDlgProc(HWND hwndDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam);

	TCHAR m_document_folder[MAX_PATH];
	TCHAR m_prefix[56];
	TCHAR m_imageFormat[56];
	bool m_running;

	TCHAR m_optionfile[MAX_PATH];
	static void setDefaultOptionsFileName(TCHAR* optionfile);
	TCHAR* getDefaultOptionsFileName();
	char m_InfoMsg[255]{ 0 };
	char m_ClassName[255]{ 0 };

	bool m_HideEndOfStreamError;

private:
	void ShowUsage(LPTSTR info = NULL);
	void FixScaling();

	HWND hwnd;
};

#endif //VNCOPTIONS_H__
