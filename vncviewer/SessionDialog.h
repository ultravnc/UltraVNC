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
	TCHAR m_host_dialog[256];
	TCHAR m_proxyhost[256];
   	virtual ~SessionDialog();

	ClientConnection *m_pCC;
	//VNCOptions *m_pOpt;
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
	bool fExitCheck;
	bool allowMonitorSpanning;
	bool changeServerRes;
	bool extendDisplay;
	bool use_virt;
	bool use_allmonitors;
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
	bool requestShapeUpdates;
	int  quickoption;
	bool ignoreShapeUpdates;
	bool Emul3Buttons; 
	bool JapKeyboard;
	bool preemptiveUpdates;
	bool FullScreen;
	bool SavePos;
	bool SaveSize;
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
	bool restricted;
	bool NoStatus;
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
	void InitDlgProc(bool loadhost = false, bool initMruNeeded = true);
	void InitDlgProcListen();

	void ReadDlgProcEncoders();
	void ReadDlgProcKeyboardMouse();
	void ReadDlgProcDisplay();
	void ReadDlgProcMisc();
	void ReadDlgProcSecurity();	
	void ReadDlgProc();
	void ReadDlgProcListen();

	HWND EncodersHwnd, KeyboardMouseHwnd, DisplayHwnd, MiscHwnd, SecurityHwnd, SessHwnd, QuickOptionsHwnd, ListenHwnd;
	void FixScaling();
	void SaveConnection(HWND hwnd, bool saveAs);
	void SettingsFromUI();
	void SettingsToUI(bool initMruNeeded = true);
	void SaveToFile(char *fname, bool SaveToFile = false);
	void saveInt(char *name, int value, char *fname); 
	void LoadFromFile(char *fname);
	int readInt(char *name, int defval, char *fname);
	void getAppData(char * buffer);
	void IfHostExistLoadSettings(char *filename);
	void SetDefaults();
	VNCOptions *m_pOpt;
	void StartListener();
	void ModeSwitch(HWND hwnd, WPARAM wParam);
private:

	int cx, cy;
	HWND hTabEncoders, hTabKeyboardMouse, hTabDisplay, hTabMisc, hTabSecurity, hTabQuickOptions, hTabListen;
	HWND m_hTab;
	HWND browser_hwnd;
	
};
