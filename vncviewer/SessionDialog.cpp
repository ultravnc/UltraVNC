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


// SessionDialog.cpp: implementation of the SessionDialog class.

#include "stdhdrs.h"
#include "vncviewer.h"
#include "SessionDialog.h"
#include "Exception.h"
#include "common/win32_helpers.h"
#include <ShlObj.h>
#include <sys/stat.h>
#include <direct.h>
#include "display.h"

#define SESSION_MRU_KEY_NAME _T("Software\\ORL\\VNCviewer\\MRU")
#define NUM_MRU_ENTRIES 8

extern char sz_F1[64];
extern char sz_F2[64];
extern char sz_F3[64];
extern char sz_F4[64];
extern char sz_F5[128];
extern char sz_F6[64];
extern char sz_F7[128];
extern char sz_F8[128];
extern char sz_F9[64];
extern char sz_F10[64];
extern char sz_F11[64];

SessionDialog::SessionDialog(VNCOptions *pOpt, ClientConnection* pCC, CDSMPlugin* pDSMPlugin)
{
	m_bExpanded = true;
	cy =0;
	cx = 0;
	m_pCC = pCC;
	m_pOpt = pOpt;
	m_pMRU = new MRU(SESSION_MRU_KEY_NAME,98);
	m_pDSMPlugin = pDSMPlugin;
	/////////////////////////////////////////////////	
	TCHAR tmphost2[256];
	_tcscpy_s(m_proxyhost, m_pOpt->m_proxyhost);
	if (strcmp(m_proxyhost,"")!=NULL) {
		_tcscat_s(m_proxyhost,":");
		_tcscat_s(m_proxyhost, 256, _itoa(m_pOpt->m_proxyport,tmphost2,10));
	}
	
	for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
		UseEnc[i] = m_pOpt->m_UseEnc[i];
	}

	PreferredEncodings.clear();
	for (int i=0; i<m_pOpt->m_PreferredEncodings.size(); i++) 
        PreferredEncodings.push_back(m_pOpt->m_PreferredEncodings[i]); 

	ViewOnly = m_pOpt->m_ViewOnly;
	fAutoScaling = m_pOpt->m_fAutoScaling;
	fExitCheck = m_pOpt->m_fExitCheck;
	m_fUseProxy = m_pOpt->m_fUseProxy;
	allowMonitorSpanning = m_pOpt->m_allowMonitorSpanning;
	changeServerRes = m_pOpt->m_ChangeServerRes;
	extendDisplay = m_pOpt->m_extendDisplay;
	use_virt = m_pOpt->m_use_virt;
	use_allmonitors = m_pOpt->m_use_allmonitors;
	requestedWidth = m_pOpt->m_requestedWidth;
	requestedHeight = m_pOpt->m_requestedHeight;
	autoDetect = m_pOpt->autoDetect;
	SwapMouse = m_pOpt->m_SwapMouse;
	DisableClipboard = m_pOpt->m_DisableClipboard;
	Use8Bit = m_pOpt->m_Use8Bit;
	Shared = m_pOpt->m_Shared;
	running = m_pOpt->m_running;
	ShowToolbar = m_pOpt->m_ShowToolbar;
	scale_num = m_pOpt->m_scale_num;
	scale_den = m_pOpt->m_scale_den;				  
	nServerScale = m_pOpt->m_nServerScale;
	reconnectcounter = m_pOpt->m_reconnectcounter;
	autoReconnect = m_pOpt->m_autoReconnect;
	FTTimeout = m_pOpt->m_FTTimeout;
	listenport = m_pOpt->m_listenPort;
	fEnableCache = m_pOpt->m_fEnableCache;
	fEnableZstd = m_pOpt->m_fEnableZstd;
	useCompressLevel = m_pOpt->m_useCompressLevel;
	enableJpegCompression = m_pOpt->m_enableJpegCompression;
	compressLevel = m_pOpt->m_compressLevel;
	jpegQualityLevel = m_pOpt->m_jpegQualityLevel;
	throttleMouse = m_pOpt->m_throttleMouse;
	requestShapeUpdates = m_pOpt->m_requestShapeUpdates;
	ignoreShapeUpdates = m_pOpt->m_ignoreShapeUpdates;
	Emul3Buttons = m_pOpt->m_Emul3Buttons;
	JapKeyboard = m_pOpt->m_JapKeyboard;
	quickoption = m_pOpt->m_quickoption;
	preemptiveUpdates = m_pOpt->m_preemptiveUpdates;
	FullScreen = m_pOpt->m_FullScreen;
	Directx = m_pOpt->m_Directx;
	SavePos = m_pOpt->m_SavePos;
	SaveSize = m_pOpt->m_SaveSize;
	fUseDSMPlugin = m_pOpt->m_fUseDSMPlugin;
	strcpy_s( szDSMPluginFilename, m_pOpt->m_szDSMPluginFilename);
	listening = m_pOpt->m_listening;
	oldplugin = m_pOpt->m_oldplugin;


	strcpy_s(folder, m_pOpt->m_document_folder);
	strcpy_s(prefix, m_pOpt->m_prefix);
	strcpy_s(imageFormat, m_pOpt->m_imageFormat);

	scaling = m_pOpt->m_scaling;


	keepAliveInterval = m_pOpt->m_keepAliveInterval;
	fAutoAcceptIncoming = m_pOpt->m_fAutoAcceptIncoming;
	fAutoAcceptNoDSM = m_pOpt->m_fAutoAcceptNoDSM;
	fRequireEncryption = m_pOpt->m_fRequireEncryption;
	restricted = m_pOpt->m_restricted;
	NoStatus = m_pOpt->m_NoStatus;
	NoHotKeys = m_pOpt->m_NoHotKeys;
	setdefaults = false;
	/////////////////////////////////////////////////
	hBmpExpand = (HBITMAP)::LoadImage(pApp->m_instance, MAKEINTRESOURCE(IDB_EXPAND), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	hBmpCollaps = (HBITMAP)::LoadImage(pApp->m_instance, MAKEINTRESOURCE(IDB_COLLAPS), IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
	hTabEncoders = NULL;
	hTabKeyboardMouse = NULL;
	hTabDisplay = NULL;
	hTabMisc = NULL;
	hTabSecurity = NULL;
	hTabQuickOptions = NULL;
	hTabListen = NULL;
	m_hTab = NULL;
	browser_hwnd = NULL;
}

SessionDialog::~SessionDialog()
{
	if (m_hTab != NULL)
		return;
	if (hTabEncoders)
		DestroyWindow(hTabEncoders);
	if (hTabKeyboardMouse)
		DestroyWindow(hTabKeyboardMouse);
	if (hTabDisplay)
		DestroyWindow(hTabDisplay);
	if (hTabMisc)
		DestroyWindow(hTabMisc);
	if (hTabSecurity)
		DestroyWindow(hTabSecurity);
	if (hTabQuickOptions)
		DestroyWindow(hTabQuickOptions);
	if (hTabListen)
		DestroyWindow(hTabListen);
    delete m_pMRU;
}

BOOL CALLBACK SessDlgProc(  HWND hwnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam );
// It's exceedingly unlikely, but possible, that if two modal dialogs were
// closed at the same time, the static variables used for transfer between 
// window procedure and this method could overwrite each other.
int SessionDialog::DoDialog()
{
 	return DialogBoxParam(pApp->m_instance, DIALOG_MAKEINTRESOURCE(IDD_SESSION_DLG), 
		NULL, (DLGPROC) SessDlgProc, (LONG_PTR) this);
}

BOOL CALLBACK SessDlgProc(  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) 
{	
	SessionDialog* _this;
    if(uMsg == WM_INITDIALOG){
      _this = (SessionDialog*)lParam;
     helper::SafeSetWindowUserData(hwnd, lParam);
    }
    else
      _this= (SessionDialog*)helper::SafeGetWindowUserData<SessionDialog>(hwnd);	                       
	switch (uMsg) {
	case WM_INITDIALOG:
		{			
            helper::SafeSetWindowUserData(hwnd, lParam);
            SessionDialog *l_this = (SessionDialog *) lParam;
			SetForegroundWindow(hwnd);
			l_this->m_fFromOptions = false;
			l_this->m_fFromFile = false;
			l_this->m_pCC->m_hSessionDialog = hwnd;
			l_this->SessHwnd = hwnd;
			_this->InitDlgProc();
			HWND hExitCheck = GetDlgItem(hwnd, IDC_EXIT_CHECK); //PGM @ Advantig
			SendMessage(hExitCheck, BM_SETCHECK, l_this->fExitCheck, 0); //PGM @ Advantig
			
			_this->ExpandBox(hwnd, !_this->m_bExpanded);						
			SendMessage(GetDlgItem(hwnd, IDC_BUTTON_EXPAND), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)_this->hBmpExpand);
            return TRUE;
		}

	case WM_NOTIFY:		
		return _this->HandleNotify(hwnd, wParam, lParam);

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_RADIOREPEATER:
			case IDC_RADIODIRECT:
				_this->ModeSwitch(hwnd, wParam);
				break;
			case IDC_HOSTNAME_EDIT: 
				if (HIWORD(wParam) == CBN_SELCHANGE) {					
					TCHAR hostname[256];
					int ItemIndex = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);
					SendMessage((HWND) lParam, (UINT) CB_GETLBTEXT, (WPARAM) ItemIndex, (LPARAM) hostname);
					_this->m_pMRU->RemoveItem(hostname);
					_this->m_pMRU->AddItem(hostname);
					_this->IfHostExistLoadSettings(hostname);
					_this->SettingsToUI(false);
				}
				if (HIWORD(wParam) == CBN_SELENDOK) {
				TCHAR hostname[256];
				int ItemIndex = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				SendMessage((HWND)lParam, (UINT)CB_SETCURSEL, (WPARAM)ItemIndex, (LPARAM)hostname);
				}
				break;
			case IDC_SAVEASDEFAULT:
				_this->SaveToFile(_this->m_pOpt->getDefaultOptionsFileName(), true);
				return true;
			case IDC_DELETE: 
					DeleteFile(_this->m_pOpt->getDefaultOptionsFileName());
					_this->SetDefaults();
					return TRUE;
			case IDC_SAVE:
				_this->SaveConnection(hwnd, false);
				break;
			case IDC_SAVEAS:
				_this->SaveConnection(hwnd, true);
				break;
			case IDCONNECT:
				_this->InitTab(hwnd);
				return _this->connect(hwnd);			
			case IDCANCEL:				
				EndDialog(hwnd, FALSE);
				return TRUE;		
			case IDC_SHOWOPTIONS:
			case IDC_BUTTON_EXPAND:
					_this->ExpandBox(hwnd, !_this->m_bExpanded);
				if (_this->m_bExpanded)
					_this->InitTab(hwnd);
				return TRUE;
			case IDC_OPTIONBUTTON:
				{				
					if (!_this->m_fFromFile){								
						HWND hExitCheck = GetDlgItem(hwnd, IDC_EXIT_CHECK); //PGM @ Advantig
						_this->fExitCheck = (SendMessage(hExitCheck, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM @ Advantig
					}
				
					//if (_this->m_pOpt->DoDialog())
					{
					_this->m_fFromOptions = true;
					 HWND hDyn = GetDlgItem(hwnd, IDC_DYNAMIC);
					SendMessage(hDyn, BM_SETCHECK, false, 0);
					HWND hLan = GetDlgItem(hwnd, IDC_LAN_RB);
					SendMessage(hLan, BM_SETCHECK, false, 0);
					HWND hUltraLan = GetDlgItem(hwnd, IDC_ULTRA_LAN_RB);
					SendMessage(hUltraLan, BM_SETCHECK, false, 0);
					HWND hMedium = GetDlgItem(hwnd, IDC_MEDIUM_RB);
					SendMessage(hMedium, BM_SETCHECK, false, 0);
					HWND hModem = GetDlgItem(hwnd, IDC_MODEM_RB);
					SendMessage(hModem, BM_SETCHECK, false, 0);
					HWND hSlow = GetDlgItem(hwnd, IDC_SLOW_RB);
					SendMessage(hSlow, BM_SETCHECK, false, 0);
					HWND hManual = GetDlgItem(hwnd, IDC_MANUAL);
					SendMessage(hManual, BM_SETCHECK, true, 0);
					_this->quickoption = 8;
					}
					return TRUE;
				}
			case IDC_LOADPROFILE_B:
				{
					TCHAR szFileName[MAX_PATH];
					memset(szFileName, '\0', MAX_PATH);
					if (_this->m_pCC->LoadConnection(szFileName, true) != -1)
					{
						TCHAR szHost[250];
						if (_this->m_pCC->m_port == 5900)
							_tcscpy_s(szHost, _this->m_pCC->m_host);
						else if (_this->m_pCC->m_port > 5900 && _this->m_pCC->m_port <= 5999)
							_snprintf_s(szHost, 250, TEXT("%s:%d"), _this->m_pCC->m_host, _this->m_pCC->m_port - 5900);
						else
							_snprintf_s(szHost, 250, TEXT("%s::%d"), _this->m_pCC->m_host, _this->m_pCC->m_port);

						SetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, szHost);
						//AaronP
						HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
						if( strcmp( _this->szDSMPluginFilename, "" ) != 0 && _this->fUseDSMPlugin ) { 
							int pos = SendMessage(hPlugins, CB_FINDSTRINGEXACT, -1,
								(LPARAM)&(_this->szDSMPluginFilename[0]));

							if( pos != CB_ERR ) {
								SendMessage(hPlugins, CB_SETCURSEL, pos, 0);
								HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
								SendMessage(hUsePlugin, BM_SETCHECK, TRUE, 0);
							}
						}
						//EndAaronP
						_this->LoadFromFile(szFileName);
						_this->SettingsToUI();
					}
					_this->m_fFromOptions = true;
					_this->m_fFromFile = true;
					return TRUE;
				}

			// [v1.0.2-jp1 fix]
			case IDC_HOSTNAME_DEL:
				HWND hcombo = GetDlgItem(  hwnd, IDC_HOSTNAME_EDIT);
				int sel = SendMessage(hcombo, CB_GETCURSEL, 0, 0);
				if(sel != CB_ERR){
					SendMessage(hcombo, CB_DELETESTRING, sel, 0);
					_this->m_pMRU->RemoveItem(sel);
				}
				return TRUE;
		}

		break;

	case WM_CLOSE:
		return FALSE;
	case WM_DESTROY:		
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return 0;
}

void SessionDialog::ExpandBox(HWND hDlg, BOOL fExpand)
{
	// if the dialog is already in the requested state, return
	// immediately.
	if (fExpand == m_bExpanded) return;

	RECT rcWnd, rcDefaultBox, rcChild, rcIntersection;
	HWND wndChild=NULL;
	HWND wndDefaultBox=NULL;
	
	// get the window of the button 
	HWND  pCtrl = GetDlgItem(hDlg, IDC_SHOWOPTIONS);
	if (pCtrl==NULL) return;

	wndDefaultBox = GetDlgItem(hDlg, IDC_DEFAULTBOX);
	if (wndDefaultBox==NULL) return;

	if (!fExpand) SendMessage(GetDlgItem(hDlg, IDC_BUTTON_EXPAND), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmpExpand);
	else SendMessage(GetDlgItem(hDlg, IDC_BUTTON_EXPAND), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmpCollaps);
	// retrieve coordinates for the default child window
	GetWindowRect(wndDefaultBox, &rcDefaultBox);

	// enable/disable all of the child window outside of the default box.
	wndChild = GetTopWindow(hDlg);

	for ( ; wndChild != NULL; wndChild = GetWindow(wndChild, GW_HWNDNEXT))
	{
		// get rectangle occupied by child window in screen coordinates.
		GetWindowRect(wndChild, &rcChild);

		if (!IntersectRect(&rcIntersection, &rcChild,&rcDefaultBox))
		{
			EnableWindow(wndChild, fExpand);
		}
	}

	if (!fExpand)  // we are contracting
	{
		_ASSERT(m_bExpanded);
		GetWindowRect(hDlg, &rcWnd);
		
		// this is the first time we are being called to shrink the dialog
		// box.  The dialog box is currently in its expanded size and we must
		// save the expanded width and height so that it can be restored
		// later when the dialog box is expanded.

		if (cx ==0 && cy == 0)
		{
			cx = rcDefaultBox.right - rcWnd.left;
			cy = rcWnd.bottom - rcWnd.top;

			// we also hide the default box here so that it is not visible
			ShowWindow(wndDefaultBox, SW_HIDE);
		}
		

		// shrink the dialog box so that it encompasses everything from the top,
		// left up to and including the default box.
		SetWindowPos(hDlg, NULL,0,0,
			rcDefaultBox.right - rcWnd.left, 
			rcDefaultBox.bottom - rcWnd.top,
			SWP_NOZORDER|SWP_NOMOVE);

		SetWindowText(pCtrl, "Show Options");

		// record that the dialog is contracted.
		m_bExpanded = FALSE;
	}
	else // we are expanding
	{
		_ASSERT(!m_bExpanded);
		SetWindowPos(hDlg, NULL,0,0,cx,cy,SWP_NOZORDER|SWP_NOMOVE);

		// make sure that the entire dialog box is visible on the user's
		// screen.
		SendMessage(hDlg, DM_REPOSITION,0,0);
		SetWindowText(pCtrl, "Hide Options");
		m_bExpanded = TRUE;
	}
}

void SessionDialog::InitPlugin(HWND hwnd)
{
				// sf@2002 - List available DSM Plugins
			HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
			int nPlugins = m_pDSMPlugin->ListPlugins(hPlugins);
			if (!nPlugins)
			{
				SendMessage(hPlugins, CB_ADDSTRING, 0, (LPARAM) sz_F11);
			}

			SendMessage(hPlugins, CB_SETCURSEL, 0, 0);

			EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), FALSE); // sf@2009 - Disable plugin config button by default			
			EnableWindow(GetDlgItem(hwnd, IDC_PLUGINS_COMBO), fUseDSMPlugin);

			//AaronP
			if( strcmp( szDSMPluginFilename, "" ) != 0 && fUseDSMPlugin )
			{ 
				int pos = SendMessage(hPlugins, CB_FINDSTRINGEXACT, -1,
					(LPARAM)&(szDSMPluginFilename[0]));

				if( pos != CB_ERR )
				{
					SendMessage(hPlugins, CB_SETCURSEL, pos, 0);
					HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
					SendMessage(hUsePlugin, BM_SETCHECK, TRUE, 0);
					EnableWindow(GetDlgItem(hwnd, IDC_PLUGIN_BUTTON), TRUE); // sf@2009 - Enable plugin config button
				}
			}
			//EndAaron
}

void SessionDialog::InitDlgProc(bool loadhost, bool initMruNeeded)
{	
	HWND hwnd = SessHwnd;		
	if (!setdefaults) {
		if (loadhost && (m_pCC->m_port != 0 || strlen(m_pCC->m_host) != 0)) {		
			TCHAR szHost[250];
			if (m_pCC->m_port == 5900)
				_tcscpy_s(szHost, m_pCC->m_host);
			else if (m_pCC->m_port > 5900 && m_pCC->m_port <= 5999)
				_snprintf_s(szHost, 250, TEXT("%s:%d"), m_pCC->m_host, m_pCC->m_port - 5900);
			else
				_snprintf_s(szHost, 250, TEXT("%s::%d"), m_pCC->m_host, m_pCC->m_port);
			SetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, szHost);
		}
		else if (initMruNeeded)
			InitMRU(hwnd);
	}
	TCHAR tmphost[256];
	TCHAR tmphost2[256];
	if (strcmp(m_proxyhost,"")!=NULL) {
		_tcscpy_s(tmphost, m_proxyhost);
		_tcscat_s(tmphost,":");
		_tcscat_s(tmphost, 256, _itoa(m_proxyport,tmphost2,10));
		SetDlgItemText(hwnd, IDC_PROXY_EDIT, tmphost);
	}
	else
		SetDlgItemText(hwnd, IDC_PROXY_EDIT, "");


	if (m_fUseProxy) {
		SendMessage(GetDlgItem(hwnd, IDC_RADIOREPEATER), BM_SETCHECK, m_fUseProxy, 0);
		ModeSwitch(hwnd, IDC_RADIOREPEATER);
	}
	else {
		SendMessage(GetDlgItem(hwnd, IDC_RADIODIRECT), BM_SETCHECK, true, 0);
		ModeSwitch(hwnd, IDC_RADIODIRECT);
	}	

	HFONT font = CreateFont(
      24,                        // nHeight
      0,                         // nWidth
      0,                         // nEscapement
      0,                         // nOrientation
      FW_BOLD,                 // nWeight
      TRUE,                     // bItalic
      FALSE,                     // bUnderline
      0,                         // cStrikeOut
      ANSI_CHARSET,              // nCharSet
      OUT_DEFAULT_PRECIS,        // nOutPrecision
      CLIP_DEFAULT_PRECIS,       // nClipPrecision
      DEFAULT_QUALITY,           // nQuality
      DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
      _T("Arial"));                 // lpszFacename

	  SendMessage(GetDlgItem(hwnd, IDC_LOGO), WM_SETFONT, (WPARAM)font, TRUE);
}

void SessionDialog::InitMRU(HWND hwnd)
{
	// Set up recently-used list
    HWND hcombo = GetDlgItem(  hwnd, IDC_HOSTNAME_EDIT);
    TCHAR valname[256];
	SendMessage(hcombo, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < m_pMRU->NumItems(); i++) {
		m_pMRU->GetItem(i, valname, 255);
		if (strlen(valname) != 0)
			SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM) valname);
	}
    SendMessage(hcombo, CB_SETCURSEL, 0, 0);

	m_pMRU->GetItem(0, valname, 255);					
	IfHostExistLoadSettings(valname);
}

bool SessionDialog::connect(HWND hwnd)
{
	SettingsFromUI();
	for (int i = rfbEncodingRaw; i <= LASTENCODING; i++) {
		m_pOpt->m_UseEnc[i] = UseEnc[i];
	}

	m_pOpt->m_PreferredEncodings.clear();
	for (int i=0; i<PreferredEncodings.size(); i++) 
        m_pOpt->m_PreferredEncodings.push_back(PreferredEncodings[i]); 

	m_pOpt->autoDetect = autoDetect;
	m_pOpt->m_fExitCheck = fExitCheck;
	m_pOpt->m_fUseProxy = m_fUseProxy;
	m_pOpt->m_allowMonitorSpanning = allowMonitorSpanning;
	m_pOpt->m_ChangeServerRes = changeServerRes;
	m_pOpt->m_extendDisplay = extendDisplay;
	m_pOpt->m_use_virt = use_virt;
	m_pOpt->m_use_allmonitors = use_allmonitors;
	m_pOpt->m_requestedWidth = requestedWidth;
	m_pOpt->m_requestedHeight = requestedHeight;
	m_pOpt->m_SwapMouse = SwapMouse;
	m_pOpt->m_DisableClipboard = DisableClipboard;
	m_pOpt->m_Use8Bit = Use8Bit;
	m_pOpt->m_Shared = Shared;
	m_pOpt->m_running = running;
	m_pOpt->m_ViewOnly = ViewOnly;
	m_pOpt->m_ShowToolbar = ShowToolbar;
	m_pOpt->m_fAutoScaling = fAutoScaling;
	m_pOpt->m_scale_num = scale_num;
	m_pOpt->m_scale_den = scale_den;
	m_pOpt->m_nServerScale = nServerScale;
	m_pOpt->m_reconnectcounter = reconnectcounter;
	m_pOpt->m_autoReconnect = autoReconnect;
	m_pOpt->m_FTTimeout = FTTimeout;
	m_pOpt->m_listenPort = listenport;
	m_pOpt->m_fEnableCache = fEnableCache;
	m_pOpt->m_fEnableZstd = fEnableZstd;
	m_pOpt->m_useCompressLevel = useCompressLevel;
	m_pOpt->m_enableJpegCompression = enableJpegCompression;
	m_pOpt->m_compressLevel = compressLevel;
	m_pOpt->m_jpegQualityLevel = jpegQualityLevel;
	m_pOpt->m_throttleMouse = throttleMouse;
	m_pOpt->m_requestShapeUpdates = requestShapeUpdates;
	m_pOpt->m_ignoreShapeUpdates = ignoreShapeUpdates;
	m_pOpt->m_Emul3Buttons = Emul3Buttons;
	m_pOpt->m_JapKeyboard = JapKeyboard;
	m_pOpt->m_quickoption = quickoption;
	m_pOpt->m_preemptiveUpdates = preemptiveUpdates;
	m_pOpt->m_FullScreen = FullScreen;
	m_pOpt->m_Directx = Directx;
	m_pOpt->m_SavePos = SavePos;
	m_pOpt->m_SaveSize = SaveSize;
	m_pOpt->m_fUseDSMPlugin = fUseDSMPlugin;
	strcpy_s( m_pOpt->m_szDSMPluginFilename, szDSMPluginFilename);
	m_pOpt->m_listening = listening;
	m_pOpt->m_oldplugin = oldplugin;
	strcpy_s(m_pOpt->m_document_folder, folder);
	strcpy_s(m_pOpt->m_prefix, prefix);
	strcpy_s(m_pOpt->m_imageFormat, imageFormat);
	m_pOpt->m_scaling = scaling;
	m_pOpt->m_keepAliveInterval = keepAliveInterval;
	m_pOpt->m_fAutoAcceptIncoming = fAutoAcceptIncoming;
	m_pOpt->m_fAutoAcceptNoDSM =fAutoAcceptNoDSM;
	m_pOpt->m_fRequireEncryption = fRequireEncryption;
	m_pOpt->m_restricted = restricted;
	m_pOpt->m_NoStatus = NoStatus;
	m_pOpt->m_NoHotKeys = NoHotKeys;


	HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
	if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED){
		TCHAR szPlugin[MAX_PATH];
		GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
		fUseDSMPlugin = true;
		strcpy_s(szDSMPluginFilename, szPlugin);
		if (!m_pDSMPlugin->IsLoaded()){
			m_pDSMPlugin->LoadPlugin(szPlugin, listening);
			if (m_pDSMPlugin->IsLoaded()){
				if (m_pDSMPlugin->InitPlugin()){
					if (!m_pDSMPlugin->SupportsMultithreaded())
						oldplugin=true; //PGM
					else //PGM
						oldplugin=false;
					m_pDSMPlugin->SetEnabled(true);
					m_pDSMPlugin->DescribePlugin();
				}
				else{
					m_pDSMPlugin->SetEnabled(false);
					MessageBox(hwnd, sz_F7, sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
					return TRUE;
					}
			}
			else{
				m_pDSMPlugin->SetEnabled(false);
				MessageBox(hwnd, sz_F5, sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
				return TRUE;
			}
		}
		else {
			// sf@2003 - If the plugin is already loaded here it has been loaded
			// by clicking on the config button: we need to init it !
			// But we must first check that the loaded plugin is the same that 
			// the one currently selected...
			m_pDSMPlugin->DescribePlugin();
			if (_stricmp(m_pDSMPlugin->GetPluginFileName(), szPlugin)){
				// Unload the previous plugin
				m_pDSMPlugin->UnloadPlugin();
				// Load the new selected one
				m_pDSMPlugin->LoadPlugin(szPlugin, listening);
			}
			if (m_pDSMPlugin->IsLoaded()){
				if (m_pDSMPlugin->InitPlugin()){
					if (!m_pDSMPlugin->SupportsMultithreaded())
						oldplugin=true; //PGM
					else //PGM
						oldplugin=false;
					m_pDSMPlugin->SetEnabled(true);
					m_pDSMPlugin->DescribePlugin();
				}
				else{
					m_pDSMPlugin->SetEnabled(false);
					MessageBox(hwnd, sz_F7, sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
					return TRUE;
				}
			}
			else{
				m_pDSMPlugin->SetEnabled(false);
				MessageBox(hwnd, sz_F5, sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
				return TRUE;
			}
		}
	}
	else { // If Use plugin unchecked but the plugin is loaded, unload it
		fUseDSMPlugin = false;
		if (m_pDSMPlugin->IsEnabled()) {
			m_pDSMPlugin->UnloadPlugin();
			m_pDSMPlugin->SetEnabled(false);
		}
	}

	TCHAR hostname[256];
	GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, hostname, 256);
		m_pMRU->AddItem(hostname);

	EndDialog(hwnd, TRUE);
	return TRUE;
}

void SessionDialog::ModeSwitch(HWND hwnd, WPARAM wParam)
{
	switch (LOWORD(wParam))
	{
	case IDC_RADIOREPEATER:
		EnableWindow(GetDlgItem(hwnd, IDC_PROXY_EDIT), true);
		ShowWindow(GetDlgItem(hwnd, IDC_HOSTNAME_EDIT), true);
		ShowWindow(GetDlgItem(hwnd, IDC_PROXY_EDIT), true);
		SetDlgItemText(hwnd, IDC_LINE1, "ID:12345679"); 
		SetDlgItemText(hwnd, IDC_LINE2, "repeater:port");	
		break;
	case IDC_RADIODIRECT:
		EnableWindow(GetDlgItem(hwnd, IDC_PROXY_EDIT), false);
		ShowWindow(GetDlgItem(hwnd, IDC_HOSTNAME_EDIT), true);
		ShowWindow(GetDlgItem(hwnd, IDC_PROXY_EDIT), true);		
		SetDlgItemText(hwnd, IDC_LINE1, "server:port");
		SetDlgItemText(hwnd, IDC_LINE2, "");
			break;
	}
}
