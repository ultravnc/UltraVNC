// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#pragma once
#include "VNCOptions.h"
#include "MRU.h"
#include <DSMPlugin/DSMPlugin.h>
#include<map>
using namespace std;

class SessionDialog  
{
public:

	// Create a connection dialog, with the options to be
	// displayed if the options.. button is clicked.
	int SetQuickOption(int quickoption);
	int ReadQuickOptionsFromUI(SessionDialog* _this, HWND hwnd);
	SessionDialog(VNCOptions *pOpt, ClientConnection* pCC, CDSMPlugin* pDSMPlugin); // sf@2002
	int DoDialog();
	int m_port;
	int m_proxyport;
	bool m_fUseProxy;
	bool m_fFromOptions; // sf@2002
	bool m_fFromFile; // sf@2002
	TCHAR m_host_dialog[MAX_HOST_NAME_LEN];
	TCHAR m_proxyhost[MAX_HOST_NAME_LEN];
   	virtual ~SessionDialog();

	ClientConnection *m_pCC;
	MRU *m_pMRU;
	CDSMPlugin* m_pDSMPlugin; // sf@2002

	void ExpandBox(HWND hDlg, BOOL fExpand);
	BOOL m_bExpanded;
	HBITMAP hBmpExpand;
	HBITMAP hBmpCollaps;
	void InitPlugin(HWND hwnd);
	void InitMRU(HWND hwnd);
	void InitTab(HWND hwnd);
	int HandleNotify(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
	int HandeleEncodersMessages(HWND hwnd, WPARAM wParam);

	bool ViewOnly;
	bool fAutoScaling;
	bool fAutoScalingEven;
	bool fAutoScalingLimit;
	bool fExitCheck;
	bool allowMonitorSpanning;
	bool changeServerRes;
	bool extendDisplay;
	bool showExtend;
	bool use_virt;
	bool useAllMonitors;
	int requestedWidth;
	int requestedHeight;
	bool UseEnc[LASTENCODING+1];
	std::vector<int> PreferredEncodings;
	bool autoDetect;
	bool SwapMouse;
	bool DisableClipboard;
	int Use8Bit;
	bool Shared;
	bool running;
	bool ShowToolbar;
	int nServerScale;
	int	scale_num, scale_den;
	int reconnectcounter, autoReconnect, FTTimeout, listenport;
	bool fEnableCache;
	bool fEnableZstd;
	bool useCompressLevel;
	int	compressLevel;
	bool enableJpegCompression;
	int	jpegQualityLevel;
	int  throttleMouse;
	bool giiEnable;
	bool requestShapeUpdates;
	int  quickoption;
	bool ignoreShapeUpdates;
	bool BlockSameMouse;
	bool Emul3Buttons; 
	bool JapKeyboard;
	bool preemptiveUpdates;
	bool FullScreen;
	bool SavePos;
	bool SaveSize;
	bool GNOME;
	bool Directx;
	bool fUseDSMPlugin;
	TCHAR szDSMPluginFilename[_MAX_PATH];
	bool oldplugin;
	bool listening;
	TCHAR folder[MAX_PATH];
	TCHAR prefix[56];
	TCHAR imageFormat[56];
	bool scaling;
	int keepAliveInterval;
	bool fAutoAcceptIncoming;
	bool fAutoAcceptNoDSM;
	bool fRequireEncryption;
	bool fUseOnlyDefaultConfigFile;
	bool restricted;
	bool ipv6;
	bool AllowUntrustedServers;
	bool NoStatus;
	bool HideEndOfStreamError;
	bool NoHotKeys;
	bool setdefaults;
	bool connect(HWND hwnd);
	map<pair<int, int>, int >resolutionMap;;

	void HandleQuickOption(int quickoption);
	void InitDlgProcEncoders();
	void InitDlgProcKeyboardMouse();
	void InitDlgProcDisplay();
	void setDisplays();
	void InitDlgProcMisc();
	void InitDlgProcSecurity();	
	void InitDlgProcConfig();
	void InitDlgProc(bool loadhost = false, bool initMruNeeded = true);
	void InitDlgProcListen();

	void ReadDlgProcEncoders();
	void ReadDlgProcKeyboardMouse();
	void ReadDlgProcDisplay();
	void ReadDlgProcMisc();
	void ReadDlgProcSecurity();	
	void ReadDlgProcConfig();
	void ReadDlgProc();
	void ReadDlgProcListen();

	HWND EncodersHwnd, KeyboardMouseHwnd, DisplayHwnd, MiscHwnd, SecurityHwnd, ConfigHwnd,
			SessHwnd, QuickOptionsHwnd, ListenHwnd;
	void FixScaling();
	void SaveConnection(HWND hwnd, bool saveAs);
	void SettingsFromUI();
	void SettingsToUI(bool initMruNeeded = true);
	void SaveToFile(char *fname, bool SaveToFile = false);
	void saveInt(char *name, int value, char *fname); 
	void LoadFromFile(char *fname);
	void overwriteCommandLine();
	int readInt(char *name, int defval, char *fname);
	void getAppData(char * buffer);
	void IfHostExistLoadSettings(char *filename);
	void SetDefaults();
	VNCOptions *m_pOpt;
	void StartListener();
	void ModeSwitch(HWND hwnd, WPARAM wParam);
	UINT m_Dpi;
	char InfoMsg[255]{0};
	char customConfigFile[_MAX_PATH]{};
private:
	HWND hTabEncoders, hTabKeyboardMouse, hTabDisplay, hTabMisc, hTabSecurity,
			hTabQuickOptions, hTabListen, hTabConfig;
	HWND m_hTab;
	HWND browser_hwnd;
	
};
