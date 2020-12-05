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
#include "stdhdrs.h"
#include "vncviewer.h"
#include "SessionDialog.h"
#include <ShlObj.h>
#include <direct.h>
#include <fstream>
extern char sz_K1[64];
extern char sz_K2[64];
extern bool g_disable_sponsor;
static OPENFILENAME ofn;

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
				MessageBox(hwnd, msg, sz_K2, MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
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
	InitMRU(hwnd);
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
}

void SessionDialog::SettingsToUI(bool initMruNeeded)
{
	InitDlgProcEncoders();
	InitDlgProcKeyboardMouse();
	InitDlgProcDisplay();
	InitDlgProcMisc();
	InitDlgProcSecurity();	
	InitDlgProcListen();
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
	saveInt("AutoScaling",          fAutoScaling,     fname);
	saveInt("fullscreen",			FullScreen,		fname);
	saveInt("SavePos",				SavePos, fname);
	saveInt("SaveSize",				SaveSize, fname);
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
	saveInt("UseProxy",				m_fUseProxy,	fname);
	saveInt("allowMonitorSpanning", allowMonitorSpanning, fname);
	saveInt("ChangeServerRes", changeServerRes, fname);
	saveInt("extendDisplay", extendDisplay, fname);
	saveInt("showExtend", showExtend, fname);
	saveInt("use_virt", use_virt, fname);	
	saveInt("useAllMonitors", use_allmonitors, fname);
	saveInt("requestedWidth", requestedWidth, fname);
	saveInt("requestedHeight", requestedHeight, fname);


	WritePrivateProfileString("options", "DSMPlugin",	szDSMPluginFilename, fname);
	WritePrivateProfileString("options", "folder",		folder, fname);
	WritePrivateProfileString("options", "prefix",		prefix, fname);
	WritePrivateProfileString("options", "imageFormat",		imageFormat, fname);
	saveInt("AutoReconnect",		autoReconnect,	fname);
	saveInt("FileTransferTimeout",  FTTimeout,    fname);
	saveInt("ThrottleMouse",		throttleMouse,    fname); 
	saveInt("KeepAliveInterval",    keepAliveInterval,    fname);	
	saveInt("AutoAcceptIncoming",	fAutoAcceptIncoming, fname);  
	saveInt("AutoAcceptNoDSM",		fAutoAcceptNoDSM, fname);
#ifdef _Gii
	saveInt("GiiEnable", giiEnable, fname);
#endif
	saveInt("RequireEncryption",	fRequireEncryption, fname);
	saveInt("restricted",			restricted,		fname);  //hide menu
	saveInt("nostatus",				NoStatus,			fname); //hide status window
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
  ViewOnly =			readInt("viewonly",			ViewOnly,		fname) != 0;
  NoStatus =			readInt("nostatus",			NoStatus,		fname) != 0;
  NoHotKeys =			readInt("nohotkeys",			NoHotKeys,	fname) != 0;
  ShowToolbar =			readInt("showtoolbar",			ShowToolbar,		fname) != 0;
  fAutoScaling =		readInt("AutoScaling",			fAutoScaling,		fname) != 0;
  FullScreen =			readInt("fullscreen",		FullScreen,	fname) != 0;
  SavePos =				readInt("SavePos", SavePos, fname) != 0;
  SaveSize =			readInt("SaveSize", SaveSize, fname) != 0;
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
  m_fUseProxy =			readInt("UseProxy",			m_fUseProxy, fname) != 0;
  allowMonitorSpanning = readInt("allowMonitorSpanning", allowMonitorSpanning, fname);
  changeServerRes = readInt("ChangeServerRes", changeServerRes, fname);
  extendDisplay = readInt("extendDisplay", extendDisplay, fname);
  showExtend = readInt("showExtend", showExtend, fname);
  use_virt = readInt("use_virt", use_virt, fname);
  use_allmonitors = readInt("useAllMonitors", use_allmonitors, fname);

  requestedWidth = readInt("requestedWidth", requestedWidth, fname);
  requestedHeight = readInt("requestedHeight", requestedHeight, fname);

  GetPrivateProfileString("options", "DSMPlugin", "NoPlugin", szDSMPluginFilename, MAX_PATH, fname);
  GetPrivateProfileString("options", "folder", folder, folder, MAX_PATH, fname);
  GetPrivateProfileString("options", "prefix", prefix, prefix, 56, fname);
  GetPrivateProfileString("options", "imageFormat", imageFormat, imageFormat, 56, fname);  
  if (!g_disable_sponsor) g_disable_sponsor=readInt("sponsor",			g_disable_sponsor, fname) != 0;
  autoReconnect =		readInt("AutoReconnect",	autoReconnect, fname);
  FTTimeout  =			readInt("FileTransferTimeout", FTTimeout, fname);
  if (FTTimeout > 600)
      FTTimeout = 600; // cap at 1 minute
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
  preemptiveUpdates = readInt("PreemptiveUpdates", (int)preemptiveUpdates, fname) ? true : false;

  GetPrivateProfileString("connection", "proxyhost", "", m_proxyhost, MAX_HOST_NAME_LEN, fname);
  m_proxyport = GetPrivateProfileInt("connection", "proxyport", 0, fname);


}

void SessionDialog::getAppData(char * buffer)
{
	BOOL result = SHGetSpecialFolderPathA( NULL, buffer, CSIDL_APPDATA, false );
}

void SessionDialog::IfHostExistLoadSettings(char *hostname)
{
	
	TCHAR tmphost[256];
	int port;
	ParseDisplay(hostname, tmphost, 255, &port);
	char fname[_MAX_PATH];
	int disp = PORT_TO_DISPLAY(port);
	sprintf_s(fname, "%.15s-%d.vnc", tmphost, (disp > 0 && disp < 100) ? disp : port);
	char buffer[_MAX_PATH];
	getAppData(buffer);
	strcat_s(buffer,"\\UltraVNC\\");
	strcat_s(buffer,fname);
	FILE *file = fopen(buffer, "r");
	if (strlen(hostname) != 0 && file ) {
		fclose(file);
		LoadFromFile(buffer);		
	}
	else
		LoadFromFile(m_pOpt->getDefaultOptionsFileName());
}

void SessionDialog::SetDefaults()
{
	SettingsFromUI();
	ViewOnly = false;
	FullScreen = false;
	SavePos = false;
	SaveSize = false;
	Directx = false;
	autoDetect = false;
	Use8Bit = rfbPFFullColors; //false;
	ShowToolbar = true;
	fAutoScaling = false;
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
	use_allmonitors =0;
	requestedWidth = 0;
	requestedHeight = 0;
	_tcscpy_s(prefix, "vnc_");
	_tcscpy_s(imageFormat, ".jpeg");
	fAutoAcceptIncoming = false;
	fAutoAcceptNoDSM = false;
	fRequireEncryption = false;
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