// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdhdrs.h"
#include "vncviewer.h"
#include "SessionDialog.h"
#include <shlobj.h>
#include <direct.h>
#include <fstream>
#include "UltraVNCHelperFunctions.h"
#include "common/win32_helpers.h"
using namespace helper;
extern HINSTANCE m_hInstResDLL;

extern char sz_K1[64];
extern char sz_K2[64];
extern bool g_disable_sponsor;
static OPENFILENAME ofn;

extern int EncodingFromString(const char* szEncoding);
void SessionDialog::SaveConnection(HWND hwnd, bool saveAs)
{
	SettingsFromUI();
	char fname[_MAX_PATH];
	int disp = PORT_TO_DISPLAY(m_port);
	sprintf_s(fname, "%.15s-%d.vnc", m_host_dialog, (disp > 0 && disp < 100) ? disp : m_port);
	char buffer[_MAX_PATH];
	getAppData(buffer);
	strcat_s(buffer,"\\UltraVNC");
	_mkdir(buffer);

	if ( saveAs) {
		char tname[_MAX_FNAME + _MAX_EXT];
		
		static char filter[] = "VNC files (*.vnc)\0*.vnc\0" \
				"All files (*.*)\0*.*\0";
		memset((void *) &ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
		ofn.lpstrFilter = filter;
		ofn.nMaxFile = _MAX_PATH;
		ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
		ofn.lpstrDefExt = "vnc";	
		ofn.hwndOwner = hwnd;
		ofn.lpstrFile = fname;
		ofn.lpstrFileTitle = tname;
		ofn.lpstrInitialDir = buffer;
		ofn.Flags = OFN_HIDEREADONLY;
		if (!GetSaveFileName(&ofn)) {
			DWORD err = CommDlgExtendedError();
			char msg[1024]; 
			switch(err) {
			case 0:	// user cancelled
				break;
			case FNERR_INVALIDFILENAME:
				strcpy_s(msg, sz_K1);
				yesUVNCMessageBox(m_hInstResDLL, hwnd, msg, sz_K2, MB_ICONERROR);
				break;
			default:
				vnclog.Print(0, "Error %d from GetSaveFileName\n", err);
				break;
			}
			return;
		}
		SaveToFile(fname);
	}
	else {
		strcat_s(buffer,"\\");
		strcat_s(buffer,fname);
		SaveToFile(buffer);
	}
	
	TCHAR hostname[256];
	GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, hostname, 256);
	m_pMRU->AddItem(hostname);
	//InitMRU(hwnd);
}

void SessionDialog::SettingsFromUI()
{
	ReadDlgProcEncoders();
	ReadDlgProcKeyboardMouse();
	ReadDlgProcDisplay();
	ReadDlgProcMisc();
	ReadDlgProcSecurity();
	ReadDlgProcListen();
	ReadDlgProc();
	ReadDlgProcConfig();
}

void SessionDialog::SettingsToUI(bool initMruNeeded)
{
	InitDlgProcEncoders();
	InitDlgProcKeyboardMouse();
	InitDlgProcDisplay();
	InitDlgProcMisc();
	InitDlgProcSecurity();	
	InitDlgProcListen();
	InitDlgProcConfig();
	InitDlgProc(true, initMruNeeded);		
}

void SessionDialog::saveInt(char *name, int value, char *fname) 
{
  char buf[10];
  sprintf_s(buf, "%d", value); 
  WritePrivateProfileString("options", name, buf, fname);
}

int SessionDialog::readInt(char *name, int defval, char *fname)
{
  return GetPrivateProfileInt("options", name, defval, fname);
}

void SessionDialog::SaveToFile(char *fname, bool asDefault)
{
	int ret;
	char buf[32];
	if (!asDefault) {
		ret = WritePrivateProfileString("connection", "host", m_host_dialog, fname);		
		sprintf_s(buf, "%d", m_port);
		WritePrivateProfileString("connection", "port", buf, fname);
	}
	else
		SettingsFromUI();
	ret = WritePrivateProfileString("connection", "proxyhost", m_proxyhost, fname);
	sprintf_s(buf, "%d", m_proxyport);
	WritePrivateProfileString("connection", "proxyport", buf, fname);
	for (int i = rfbEncodingRaw; i<= LASTENCODING; i++) {
		char buf[128];
		sprintf_s(buf, "use_encoding_%d", i);
		saveInt(buf, UseEnc[i], fname);
	 }
	if (!PreferredEncodings.empty()) {
	  saveInt("preferred_encoding", PreferredEncodings[0], fname);
	}	
	saveInt("viewonly",				ViewOnly,			fname);	
	saveInt("showtoolbar",			ShowToolbar,		fname);
	saveInt("fullscreen",			FullScreen,		fname);
	saveInt("SavePos",				SavePos, fname);
	saveInt("SaveSize",				SaveSize, fname);
	saveInt("GNOME",				GNOME, fname);
	saveInt("directx",				Directx,		fname);
	saveInt("autoDetect",			autoDetect, fname);
	saveInt("8bit",					Use8Bit,			fname);
	saveInt("shared",				Shared,			fname);
	saveInt("swapmouse",			SwapMouse,		fname);
	saveInt("emulate3",				Emul3Buttons,		fname);
	saveInt("JapKeyboard",			JapKeyboard,		fname);
	saveInt("disableclipboard",		DisableClipboard, fname);
	saveInt("Scaling",				scaling,		fname);
	saveInt("AutoScaling",			fAutoScaling,		fname);
	saveInt("AutoScalingEven",      fAutoScalingEven, fname);
	saveInt("AutoScalingLimit",		fAutoScalingLimit, fname);
	saveInt("scale_num",			scale_num,		fname);
	saveInt("scale_den",			scale_den,		fname);
	// Tight Specific
	saveInt("cursorshape",			requestShapeUpdates, fname);
	saveInt("noremotecursor",		ignoreShapeUpdates, fname);
	if (useCompressLevel) {
		saveInt("compresslevel",	compressLevel,	fname);
	}
	if (enableJpegCompression) {
		saveInt("quality",			jpegQualityLevel,	fname);
	}
	// Modif sf@2002
	saveInt("ServerScale",			nServerScale,		fname);
	saveInt("Reconnect",			reconnectcounter,		fname);
	saveInt("EnableCache",			fEnableCache,		fname);
	saveInt("EnableZstd",			fEnableZstd, fname);
	saveInt("QuickOption",			quickoption,	fname);
	saveInt("UseDSMPlugin",			fUseDSMPlugin,	fname);
	saveInt("UseProxy",				(int)m_connectionType,	fname);
	saveInt("allowMonitorSpanning", allowMonitorSpanning, fname);
	saveInt("ChangeServerRes", changeServerRes, fname);
	saveInt("extendDisplay", extendDisplay, fname);
	saveInt("showExtend", showExtend, fname);
	saveInt("use_virt", use_virt, fname);	
	saveInt("useAllMonitors", useAllMonitors, fname);
	saveInt("requestedWidth", requestedWidth, fname);
	saveInt("requestedHeight", requestedHeight, fname);


	WritePrivateProfileString("options", "DSMPlugin",	szDSMPluginFilename, fname);
	WritePrivateProfileString("options", "folder",		folder, fname);
	WritePrivateProfileString("options", "prefix",		prefix, fname);
	WritePrivateProfileString("options", "imageFormat",		imageFormat, fname);
	WritePrivateProfileString("options", "InfoMsg", InfoMsg, fname);
	saveInt("AutoReconnect",		autoReconnect,	fname);
	saveInt("FileTransferTimeout",  FTTimeout,    fname);
	saveInt("ListenPort", listenport, fname);
	saveInt("ThrottleMouse",		throttleMouse,    fname); 
	saveInt("KeepAliveInterval",    keepAliveInterval,    fname);	
	saveInt("AutoAcceptIncoming",	fAutoAcceptIncoming, fname);  
	saveInt("AutoAcceptNoDSM",		fAutoAcceptNoDSM, fname);
#ifdef _Gii
	saveInt("GiiEnable", giiEnable, fname);
#endif
	saveInt("RequireEncryption",	fRequireEncryption, fname);
	saveInt("UseOnlyDefaultConfigFile", fUseOnlyDefaultConfigFile, fname);
	saveInt("restricted",			restricted,		fname);  //hide menu
	saveInt("ipv6",					ipv6, fname);  //hide menu
	saveInt("AllowUntrustedServers", AllowUntrustedServers, fname);
	saveInt("nostatus",				NoStatus,			fname); //hide status window
	saveInt("HideEOStreamError",    HideEndOfStreamError, fname); // hide End of Stream error
	saveInt("nohotkeys",			NoHotKeys,		fname); //disable hotkeys
	saveInt("sponsor",				g_disable_sponsor,	fname);
	saveInt("PreemptiveUpdates",	preemptiveUpdates, fname);
}
void SessionDialog::LoadFromFile(char *fname)
{
  for (int i = rfbEncodingRaw; i<= LASTENCODING; i++) {
    char buf[128];
    sprintf_s(buf, "use_encoding_%d", i);
    UseEnc[i] =   readInt(buf, UseEnc[i], fname) != 0;
  }
  int nExistingPreferred = PreferredEncodings.empty() ? rfbEncodingZRLE : PreferredEncodings[0];
  int nPreferredEncoding =	readInt("preferred_encoding", nExistingPreferred,	fname);
  PreferredEncodings.clear();
  PreferredEncodings.push_back(nPreferredEncoding);

  restricted =			readInt("restricted",		restricted,	fname) != 0 ;
  ipv6 =				readInt("ipv6",				ipv6, fname) != 0;
  AllowUntrustedServers = readInt("AllowUntrustedServers", AllowUntrustedServers, fname) != 0;
  ViewOnly =			readInt("viewonly",			ViewOnly,		fname) != 0;
  NoStatus =			readInt("nostatus",			NoStatus,		fname) != 0;
  NoHotKeys =			readInt("nohotkeys",			NoHotKeys,	fname) != 0;
  ShowToolbar =			readInt("showtoolbar",			ShowToolbar,		fname) != 0;
  FullScreen =			readInt("fullscreen",		FullScreen,	fname) != 0;
  SavePos =				readInt("SavePos", SavePos, fname) != 0;
  SaveSize =			readInt("SaveSize", SaveSize, fname) != 0;
  GNOME =				readInt("GNOME", GNOME, fname) != 0;
  Directx =				readInt("directx",		Directx,	fname) != 0;
  autoDetect =			readInt("autoDetect", autoDetect, fname) != 0;
  Use8Bit =				readInt("8bit",				Use8Bit,		fname);
  Shared =				readInt("shared",			Shared,		fname) != 0;
  SwapMouse =			readInt("swapmouse",		SwapMouse,	fname) != 0;
  Emul3Buttons =		readInt("emulate3",			Emul3Buttons, fname) != 0;
  JapKeyboard  =		readInt("JapKeyboard",			JapKeyboard, fname) != 0;
  DisableClipboard =	readInt("disableclipboard", DisableClipboard, fname) != 0;
  scaling =				readInt("Scaling", scaling,  fname) != 0;
  fAutoScaling =		readInt("AutoScaling", fAutoScaling,  fname) != 0;
  fAutoScalingEven =    readInt("AutoScalingEven", fAutoScalingEven, fname) != 0;
  fAutoScalingLimit =	readInt("AutoScalingLimit", fAutoScalingLimit, fname) != 0;
  scale_num =			readInt("scale_num",		scale_num,	fname);
  scale_den =			readInt("scale_den",		scale_den,	fname);
  // Tight specific
  requestShapeUpdates =	readInt("cursorshape",		requestShapeUpdates, fname) != 0;
  ignoreShapeUpdates =	readInt("noremotecursor",	ignoreShapeUpdates, fname) != 0;
  int level =			readInt("compresslevel",	-1,				fname);
  if (level != -1) {
	useCompressLevel = true;
	compressLevel = level;
  }
  level =				readInt("quality",			-1,				fname);
  if (level != -1) {
	enableJpegCompression = true;
	jpegQualityLevel = level;
  }
  // Modif sf@2002
  nServerScale =		readInt("ServerScale",		nServerScale,	fname);
  reconnectcounter =	readInt("Reconnect",		reconnectcounter,	fname);
  fEnableCache =		readInt("EnableCache",		fEnableCache,	fname) != 0;
  fEnableZstd =			readInt("EnableZstd",		fEnableZstd, fname);
  quickoption  =		readInt("QuickOption",		quickoption, fname);
  fUseDSMPlugin =		readInt("UseDSMPlugin",		fUseDSMPlugin, fname) != 0;
  m_connectionType =			(ConnectionType)readInt("UseProxy",			(int)m_connectionType, fname);
  allowMonitorSpanning = readInt("allowMonitorSpanning", allowMonitorSpanning, fname);
  changeServerRes = readInt("ChangeServerRes", changeServerRes, fname);
  extendDisplay = readInt("extendDisplay", extendDisplay, fname);
  showExtend = readInt("showExtend", showExtend, fname);
  use_virt = readInt("use_virt", use_virt, fname);
  useAllMonitors = readInt("useAllMonitors", useAllMonitors, fname);

  requestedWidth = readInt("requestedWidth", requestedWidth, fname);
  requestedHeight = readInt("requestedHeight", requestedHeight, fname);

  GetPrivateProfileString("options", "DSMPlugin", "NoPlugin", szDSMPluginFilename, MAX_PATH, fname);
  GetPrivateProfileString("options", "folder", folder, folder, MAX_PATH, fname);
  GetPrivateProfileString("options", "prefix", prefix, prefix, 56, fname);
  GetPrivateProfileString("options", "imageFormat", imageFormat, imageFormat, 56, fname);  
  GetPrivateProfileString("options", "InfoMsg", InfoMsg, InfoMsg, 254, fname);
  if (!g_disable_sponsor) g_disable_sponsor=readInt("sponsor",			g_disable_sponsor, fname) != 0;
  autoReconnect =		readInt("AutoReconnect",	autoReconnect, fname);
  FTTimeout  =			readInt("FileTransferTimeout", FTTimeout, fname);
  if (FTTimeout > 600)
      FTTimeout = 600; // cap at 1 minute
  listenport = readInt("ListenPort", listenport, fname);
  keepAliveInterval  =	readInt("KeepAliveInterval", keepAliveInterval, fname);
  if (keepAliveInterval >= (FTTimeout - KEEPALIVE_HEADROOM))
      keepAliveInterval = (FTTimeout  - KEEPALIVE_HEADROOM); 
  throttleMouse = readInt("ThrottleMouse", throttleMouse, fname); // adzm 2010-10
#ifdef _Gii
  giiEnable = readInt("GiiEnable", (int)giiEnable, fname) ? true : false;
#endif
  fAutoAcceptIncoming = readInt("AutoAcceptIncoming", (int)fAutoAcceptIncoming, fname) ? true : false;
  fAutoAcceptNoDSM = readInt("AutoAcceptNoDSM", (int)fAutoAcceptNoDSM, fname) ? true : false;
  fRequireEncryption = readInt("RequireEncryption", (int)fRequireEncryption, fname) ? true : false;
  fUseOnlyDefaultConfigFile = readInt("UseOnlyDefaultConfigFile", (int)fUseOnlyDefaultConfigFile, fname) ? true : false;
  preemptiveUpdates = readInt("PreemptiveUpdates", (int)preemptiveUpdates, fname) ? true : false;

  GetPrivateProfileString("connection", "proxyhost", "", m_proxyhost, MAX_HOST_NAME_LEN, fname);
  m_proxyport = GetPrivateProfileInt("connection", "proxyport", 0, fname);
  overwriteCommandLine();

}

void SessionDialog::overwriteCommandLine() 
{
	char* szCmdLine = m_pOpt->szCmdLine;
	int cmdlinelen = _tcslen(szCmdLine);
	if (cmdlinelen == 0) return;

	TCHAR* cmd = new TCHAR[cmdlinelen + 1];
	_tcscpy_s(cmd, cmdlinelen + 1, szCmdLine);

	// Count the number of spaces
	// This may be more than the number of arguments, but that doesn't matter.
	int nspaces = 0;
	TCHAR* p = cmd;
	TCHAR* pos = cmd;
	while ((pos = _tcschr(p, ' ')) != NULL) {
		nspaces++;
		p = pos + 1;
	}

	// Create the array to hold pointers to each bit of string
	TCHAR** args = new LPTSTR[nspaces + 1];

	// replace spaces with nulls and
	// create an array of TCHAR*'s which points to start of each bit.
	pos = cmd;
	int i = 0;
	args[i] = cmd;
	bool inquote = false;
	for (pos = cmd; *pos != 0; pos++) {
		// Arguments are normally separated by spaces, unless there's quoting
		if ((*pos == ' ') && !inquote) {
			*pos = '\0';
			p = pos + 1;
			args[++i] = p;
		}
		if (*pos == '"') {
			if (!inquote) {      // Are we starting a quoted argument?
				args[i] = ++pos; // It starts just after the quote
			}
			else {
				*pos = '\0';     // Finish a quoted argument?
			}
			inquote = !inquote;
		}
	}
	i++;

	bool hostGiven = false, portGiven = false;
	// take in order.
	for (int j = 0; j < i; j++) {
		if (SwitchMatch(args[j], _T("listen")))
		{
			listening = true;
			if (j + 1 < i && args[j + 1][0] >= '0' && args[j + 1][0] <= '9') {
				if (_stscanf_s(args[j + 1], _T("%d"), &listenport) != 1) {
					continue;
				}
				j++;
			}
		}
		else if (SwitchMatch(args[j], _T("fttimeout"))) { //PGM @ Advantig
			if (j + 1 < i && args[j + 1][0] >= '0' && args[j + 1][0] <= '9') {
				if (_stscanf_s(args[j + 1], _T("%d"), &FTTimeout) != 1) {
					continue;
				}
				if (FTTimeout > 600)
					FTTimeout = 600;
				j++;
			}
		}
		else if (SwitchMatch(args[j], _T("keepalive"))) { //PGM @ Advantig
			if (j + 1 < i && args[j + 1][0] >= '0' && args[j + 1][0] <= '9') {
				if (_stscanf_s(args[j + 1], _T("%d"), &keepAliveInterval) != 1) {
					continue;
				}
				if (keepAliveInterval >= (FTTimeout - KEEPALIVE_HEADROOM))
					keepAliveInterval = (FTTimeout - KEEPALIVE_HEADROOM);
				j++;
			}
		}
		else if (SwitchMatch(args[j], _T("socketkeepalivetimeout"))) { // adzm 2010-08
			if (j + 1 < i && args[j + 1][0] >= '0' && args[j + 1][0] <= '9') {
				int m_socketKeepAliveTimeout;
				if (_stscanf_s(args[j + 1], _T("%d"), &m_socketKeepAliveTimeout) != 1) {
					continue;
				}
				j++;
			}
		}
		else if (SwitchMatch(args[j], _T("askexit"))) { //PGM @ Advantig
			fExitCheck = true; //PGM @ Advantig
		}
		else if (SwitchMatch(args[j], _T("restricted"))) {
			restricted = true;
		}
		else if (SwitchMatch(args[j], _T("ipv6"))) {
			ipv6 = true;
		}
		else if (SwitchMatch(args[j], _T("AllowUntrustedServers"))) {
			AllowUntrustedServers = true;
		}
		else if (SwitchMatch(args[j], _T("viewonly"))) {
			ViewOnly = true;
		}
		else if (SwitchMatch(args[j], _T("nostatus"))) {
			NoStatus = true;
		}
		else if (SwitchMatch(args[j], _T("hideendofstreamerror"))) {
			HideEndOfStreamError = true;
		}
		else if (SwitchMatch(args[j], _T("nohotkeys"))) {
			NoHotKeys = true;
		}
		else if (SwitchMatch(args[j], _T("notoolbar"))) {
			ShowToolbar = false;
		}
		else if (SwitchMatch(args[j], _T("autoscaling"))) {
			fAutoScaling = true;
		}
		else if (SwitchMatch(args[j], _T("fullscreen"))) {
			FullScreen = true;
		}
		else if (SwitchMatch(args[j], _T("savepos"))) {
			SavePos = true;
		}
		else if (SwitchMatch(args[j], _T("savesize"))) {
			SaveSize = true;
		}
		else if (SwitchMatch(args[j], _T("gnome"))) {
			GNOME = true;
		}
		else if (SwitchMatch(args[j], _T("directx"))) {
			Directx = true;
		}
		else if (SwitchMatch(args[j], _T("noauto"))) {
			autoDetect = false;
			quickoption = 0;
		}
		else if (SwitchMatch(args[j], _T("8bit"))) {
			Use8Bit = rfbPF256Colors; //true;
		}
		else if (SwitchMatch(args[j], _T("256colors"))) {
			Use8Bit = rfbPF256Colors; //true;
		}
		else if (SwitchMatch(args[j], _T("fullcolors"))) {
			Use8Bit = rfbPFFullColors;
		}
		else if (SwitchMatch(args[j], _T("64colors"))) {
			Use8Bit = rfbPF64Colors;
		}
		else if (SwitchMatch(args[j], _T("8colors"))) {
			Use8Bit = rfbPF8Colors;
		}
		else if (SwitchMatch(args[j], _T("8greycolors"))) {
			Use8Bit = rfbPF8GreyColors;
		}
		else if (SwitchMatch(args[j], _T("4greycolors"))) {
			Use8Bit = rfbPF4GreyColors;
		}
		else if (SwitchMatch(args[j], _T("2greycolors"))) {
			Use8Bit = rfbPF2GreyColors;
		}
		else if (SwitchMatch(args[j], _T("shared"))) {
			Shared = true;
		}
		else if (SwitchMatch(args[j], _T("swapmouse"))) {
			SwapMouse = true;
		}
		else if (SwitchMatch(args[j], _T("emulate3"))) {
			Emul3Buttons = true;
		}
		else if (SwitchMatch(args[j], _T("JapKeyboard"))) {
			JapKeyboard = true;
		}
		else if (SwitchMatch(args[j], _T("noemulate3"))) {
			Emul3Buttons = false;
		}
		else if (SwitchMatch(args[j], _T("nocursorshape"))) {
			requestShapeUpdates = false;
		}
		else if (SwitchMatch(args[j], _T("noremotecursor"))) {
			requestShapeUpdates = true;
			ignoreShapeUpdates = true;
		}
		else if (SwitchMatch(args[j], _T("scale"))) {
			if (++j == i) {
				continue;
			}
			int numscales = _stscanf_s(args[j], _T("%d/%d"), &scale_num, &scale_den);
			if (numscales < 1) {
				continue;
			}
			if (numscales == 1)
				scale_den = 1; // needed if you're overriding a previous setting
		}
		else if (SwitchMatch(args[j], _T("disableclipboard"))) {
			DisableClipboard = true;
		}
		else if (SwitchMatch(args[j], _T("InfoMsg"))) {
			if (++j == i) {
				continue;
			}
			strcpy_s(InfoMsg, args[j]);
		}

		else if (SwitchMatch(args[j], _T("register"))) {
			//      Register();
			PostQuitMessage(0);
		}
		else if (SwitchMatch(args[j], _T("encoding"))) {
			if (++j == i) {
				continue;
			}
			int enc = EncodingFromString(args[j]);
			if (enc == -1) {
				continue;
			}
			else {
				PreferredEncodings.clear();
				PreferredEncodings.push_back(enc);
				UseEnc[enc] = true;
			}
		}
		else if (SwitchMatch(args[j], _T("encodings"))) {
			if (++j == i) {
				continue;
			}
			int encodings_found = 0;
			while (encodings_found >= 0) {
				int enc = EncodingFromString(args[j]);
				if (enc == -1) {
					if (encodings_found == 0) {
					}
					else {
						j--;
					}
					encodings_found = -1;
				}
				else {
					if (encodings_found == 0) {
						PreferredEncodings.clear();
					}
					UseEnc[enc] = true;
					if (PreferredEncodings.end() == std::find(PreferredEncodings.begin(), PreferredEncodings.end(), enc)) {
						PreferredEncodings.push_back(enc);
					}
					encodings_found++;

					j++;

					if (j == i) {
						encodings_found = -1;
					}
				}
			}
		}
		// Tight options
		else if (SwitchMatch(args[j], _T("compresslevel"))) {
			if (++j == i) {
				continue;
			}
			useCompressLevel = true;
			if (_stscanf_s(args[j], _T("%d"), &compressLevel) != 1) {
				continue;
			}
		}
		else if (SwitchMatch(args[j], _T("quality"))) {
			if (++j == i) {
				continue;
			}
			enableJpegCompression = true;
			if (_stscanf_s(args[j], _T("%d"), &jpegQualityLevel) != 1) {
				continue;
			}
		}
		else if (SwitchMatch(args[j], _T("serverscale")))
		{
			if (++j == i)
			{
				continue;
			}
			_stscanf_s(args[j], _T("%d"), &nServerScale);
			if (nServerScale < 1 || nServerScale > 9) nServerScale = 1;
		}
		// Modif sf@2002
		else if (SwitchMatch(args[j], _T("quickoption")))
		{
			if (++j == i)
			{
				continue;
			}
			_stscanf_s(args[j], _T("%d"), &quickoption);
		}
		// Modif sf@2002 - DSM Plugin
		else if (SwitchMatch(args[j], _T("dsmplugin")))
		{
			if (++j == i)
			{
				continue;
			}
			fUseDSMPlugin = true;
			strcpy_s(szDSMPluginFilename, args[j]);
		}
		else if (SwitchMatch(args[j], _T("reconnectcounter")))
		{
			if (++j == i) {
				PostQuitMessage(1);
				continue;
			}
			_stscanf_s(args[j], _T("%d"), &reconnectcounter);
		}
		else if (SwitchMatch(args[j], _T("autoreconnect")))
		{
			if (++j == i) {
				PostQuitMessage(1);
				continue;
			}
			_stscanf_s(args[j], _T("%d"), &autoReconnect);
		}
		else if (SwitchMatch(args[j], _T("disablesponsor")))
		{
			//adzm - 2009-06-21
			g_disable_sponsor = true;
		}
		else if (SwitchMatch(args[j], _T("autoacceptincoming")))
		{
			//adzm - 2009-06-21
			fAutoAcceptIncoming = true;
		}
#ifdef _Gii
		else if (SwitchMatch(args[j], _T("giienable")))
		{
			giiEnable = true;
		}
#endif
		else if (SwitchMatch(args[j], _T("autoacceptnodsm")))
		{
			//adzm 2009-07-19
			fAutoAcceptNoDSM = true;
		}
		else if (SwitchMatch(args[j], _T("requireencryption")))
		{
			//adzm 2010-05-12
			fRequireEncryption = true;
		}
		else if (SwitchMatch(args[j], _T("preemptiveupdates")))
		{
			//adzm 2010-07-04
			preemptiveUpdates = true;
		}
		else if (SwitchMatch(args[j], _T("enablecache")))
		{
			//adzm 2010-08
			fEnableCache = true;
		}
		else if (SwitchMatch(args[j], _T("throttlemouse")))
		{
			//adzm 2010-10
			if (++j == i) {
				continue;
			}
			if (_stscanf_s(args[j], _T("%d"), &throttleMouse) != 1) {
				continue;
			}
		}
	}

	if (scale_num != 1 || scale_den != 1)
		scaling = true;

	// reduce scaling factors by greatest common denominator
	if (scaling) {
		FixScaling();
	}
	// tidy up
	delete[] cmd;
	delete[] args;
}

void SessionDialog::getAppData(char * buffer)
{
	BOOL result = SHGetSpecialFolderPathA( NULL, buffer, CSIDL_APPDATA, false );
}

void SessionDialog::IfHostExistLoadSettings(char *hostname)
{
	//SetDefaults();
	TCHAR tmphost[MAX_HOST_NAME_LEN];
	int port;
	ParseDisplay(hostname, tmphost, MAX_HOST_NAME_LEN, &port);
	char fname[_MAX_PATH];
	int disp = PORT_TO_DISPLAY(port);
	sprintf_s(fname, "%.15s-%d.vnc", tmphost, (disp > 0 && disp < 100) ? disp : port);
	char buffer[_MAX_PATH];
	getAppData(buffer);
	strcat_s(buffer,"\\UltraVNC\\");
	strcat_s(buffer,fname);
	FILE *file = fopen(buffer, "r");
	strcpy_s(customConfigFile, buffer);
	if (strlen(hostname) != 0 && file && !m_pOpt->m_UseOnlyDefaultConfigFile) {
		fclose(file);
		SetDefaults();
		LoadFromFile(m_pOpt->getDefaultOptionsFileName());;
		LoadFromFile(buffer);
		SetWindowText(GetDlgItem(hTabConfig, IDC_CUSTOMCONFIG),customConfigFile);
	}
	else {
		LoadFromFile(m_pOpt->getDefaultOptionsFileName());
		SetWindowText(GetDlgItem(hTabConfig, IDC_CUSTOMCONFIG), "");
	}
	InitDlgProcListen();
}

void SessionDialog::SetDefaults()
{
	SettingsFromUI();
	ViewOnly = false;
	FullScreen = false;
	SavePos = false;
	SaveSize = false;
	GNOME = false;
	Directx = false;
	autoDetect = false;
	Use8Bit = rfbPFFullColors; //false;
	ShowToolbar = true;
	NoStatus = false;
	NoHotKeys = false;
	PreferredEncodings.clear();
	PreferredEncodings.push_back(rfbEncodingUltra2);
	JapKeyboard = false;
	SwapMouse = false;
	Emul3Buttons = true;
	Shared = true;
	DisableClipboard = false;
	scaling = false;
	fAutoScaling = false;
	fAutoScalingEven = false;
	fAutoScalingLimit = false;
	scale_num = 100;
	scale_den = 100;  
	// Modif sf@2002 - Server Scaling
	nServerScale = 1;
	reconnectcounter = 3;
	fEnableCache = false;
	fEnableZstd = true;
	listening = false;
	listenport = INCOMING_PORT_OFFSET;
	restricted = false;
	ipv6 = false;
	AllowUntrustedServers = false;
	// Tight specific
	useCompressLevel = true;
	compressLevel = 6;		
	enableJpegCompression = true;
	jpegQualityLevel = 8;
	requestShapeUpdates = true;
	ignoreShapeUpdates = false;
	quickoption = 1;				// sf@2002 - Auto Mode as default
	fUseDSMPlugin = false;
	oldplugin=false;
	allowMonitorSpanning = 0;
	changeServerRes = 0;
	extendDisplay = 0;
	showExtend = 0;
	use_virt = 0;
	useAllMonitors =0;
	requestedWidth = 0;
	requestedHeight = 0;
	_tcscpy_s(prefix, "ultravnc_");
	_tcscpy_s(imageFormat, ".jpeg");
	fAutoAcceptIncoming = false;
	fAutoAcceptNoDSM = false;
	fRequireEncryption = false;
	fUseOnlyDefaultConfigFile = true;
	preemptiveUpdates = false;
	scale_num = 100;
	scale_den = 100;
	scaling = false; 
	autoReconnect = 3; 
	fExitCheck = false; //PGM @ Advantig
	FTTimeout = FT_RECV_TIMEOUT;
	keepAliveInterval = KEEPALIVE_INTERVAL;
	throttleMouse = 0; // adzm 2010-10 
	setdefaults = true;
	SettingsToUI();
	setdefaults = false;
	giiEnable = false;
}