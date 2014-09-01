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
#include <shlobj.h>
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
	m_pCC = pCC;
	m_pOpt = pOpt;
    //AaronP
//	m_pMRU = new MRU(SESSION_MRU_KEY_NAME);
	m_pMRU = new MRU(SESSION_MRU_KEY_NAME,26);
	//EndAaronP
	m_pDSMPlugin = pDSMPlugin;
}

SessionDialog::~SessionDialog()
{
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

static bool notset=true;
static int selected=1;

BOOL CALLBACK SessDlgProc(  HWND hwnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam ) {
	// This is a static method, so we don't know which instantiation we're 
	// dealing with. But we can get a pseudo-this from the parameter to 
	// WM_INITDIALOG, which we therafter store with the window and retrieve
	// as follows:
    /*SessionDialog*_this = helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	if (_this!=NULL && notset) 
		{
			selected=_this->m_pOpt->m_selected_screen;
			notset=false;
		}*/

	SessionDialog* _this;
    if(uMsg == WM_INITDIALOG){
      _this = (SessionDialog*)lParam;
     helper::SafeSetWindowUserData(hwnd, lParam);
    }
    else
      _this= (SessionDialog*)helper::SafeGetWindowUserData<SessionDialog>(hwnd);
	if (_this!=NULL && notset) 
		{
			selected=_this->m_pOpt->m_selected_screen;
			notset=false;
		}




	switch (uMsg) {

	case WM_INITDIALOG:
		{

            helper::SafeSetWindowUserData(hwnd, lParam);
            SessionDialog *l_this = (SessionDialog *) lParam;
            //CentreWindow(hwnd);
			SetForegroundWindow(hwnd);
			l_this->m_pCC->m_hSessionDialog = hwnd;

			//List monitors
			tempdisplayclass tdc;
			tdc.Init();
			HWND hcomboscreen = GetDlgItem(  hwnd, IDC_SCREEN);		
            for (int i = 0; i < tdc.nr_monitors+1; i++) {
                int pos = SendMessage(hcomboscreen, CB_ADDSTRING, 0, (LPARAM) tdc.monarray[i].buttontext);

            }
            SendMessage(hcomboscreen, CB_SETCURSEL, selected, 0);

            // Set up recently-used list
            HWND hcombo = GetDlgItem(  hwnd, IDC_HOSTNAME_EDIT);
            TCHAR valname[256];

            for (int i = 0; i < l_this->m_pMRU->NumItems(); i++) {
                l_this->m_pMRU->GetItem(i, valname, 255);
                int pos = SendMessage(hcombo, CB_ADDSTRING, 0, (LPARAM) valname);

            }
            SendMessage(hcombo, CB_SETCURSEL, 0, 0);

			// sf@2002 - List available DSM Plugins
			HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
			int nPlugins = l_this->m_pDSMPlugin->ListPlugins(hPlugins);
			if (!nPlugins)
			{
				SendMessage(hPlugins, CB_ADDSTRING, 0, (LPARAM) sz_F11);
			}
			else
			{
				// Use the first detected plugin, so the user doesn't have to check the option
				// HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
				// SendMessage(hUsePlugin, BM_SETCHECK, TRUE, 0);
			}
			SendMessage(hPlugins, CB_SETCURSEL, 0, 0);

			HWND hButton = GetDlgItem(hwnd, IDC_PLUGIN_BUTTON);
			EnableWindow(hButton, FALSE); // sf@2009 - Disable plugin config button by default

			//AaronP
			if( strcmp( l_this->m_pOpt->m_szDSMPluginFilename, "" ) != 0 && l_this->m_pOpt->m_fUseDSMPlugin )
			{ 
				int pos = SendMessage(hPlugins, CB_FINDSTRINGEXACT, -1,
					(LPARAM)&(l_this->m_pOpt->m_szDSMPluginFilename[0]));

				if( pos != CB_ERR )
				{
					SendMessage(hPlugins, CB_SETCURSEL, pos, 0);
					HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
					SendMessage(hUsePlugin, BM_SETCHECK, TRUE, 0);
					EnableWindow(hButton, TRUE); // sf@2009 - Enable plugin config button
				}
			}
			//EndAaronP

			TCHAR tmphost[256];
			TCHAR tmphost2[256];
			_tcscpy(tmphost, l_this->m_pOpt->m_proxyhost);
			if (strcmp(tmphost,"")!=NULL)
			{
			_tcscat(tmphost,":");
			_tcscat(tmphost,_itoa(l_this->m_pOpt->m_proxyport,tmphost2,10));
			SetDlgItemText(hwnd, IDC_PROXY_EDIT, tmphost);
			}

			HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY_CHECK);
			SendMessage(hViewOnly, BM_SETCHECK, l_this->m_pOpt->m_ViewOnly, 0);

			HWND hAutoScaling = GetDlgItem(hwnd, IDC_AUTOSCALING_CHECK);
			SendMessage(hAutoScaling, BM_SETCHECK, l_this->m_pOpt->m_fAutoScaling, 0);

			HWND hExitCheck = GetDlgItem(hwnd, IDC_EXIT_CHECK); //PGM @ Advantig
			SendMessage(hExitCheck, BM_SETCHECK, l_this->m_pOpt->m_fExitCheck, 0); //PGM @ Advantig

			HWND hProxy = GetDlgItem(hwnd, IDC_PROXY_CHECK);
			SendMessage(hProxy, BM_SETCHECK, l_this->m_pOpt->m_fUseProxy, 0);


			// sf@2005 - Make the save settings optional but always enabled by default (for now)
			// (maybe disabled as default is better ?)
			HWND hSave = GetDlgItem(hwnd, IDC_SETDEFAULT_CHECK);
//ACT			SendMessage(hSave, BM_SETCHECK, true, 0);


			// sf@2002 - Select Modem Option as default
			l_this->SetQuickOption(l_this, hwnd);

			l_this->m_fFromOptions = false;
			l_this->m_fFromFile = false;

            return TRUE;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
	    case IDC_DELETE:
			{
				char optionfile[MAX_PATH];
				VNCOptions::GetDefaultOptionsFileName(optionfile);
				DeleteFile(optionfile);
			}
			return TRUE;

		case IDOK:
			{
            TCHAR tmphost[256];
            TCHAR display[256];
            TCHAR fulldisplay[256];

			// sf@2005
			HWND hSave = GetDlgItem(hwnd, IDC_SETDEFAULT_CHECK);
			_this->m_pCC->saved_set = SendMessage(hSave, BM_GETCHECK, 0, 0) == BST_CHECKED;

			_this->m_pOpt->m_selected_screen=SendMessage(GetDlgItem(  hwnd, IDC_SCREEN),CB_GETCURSEL,0,0);

			GetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, display, 256);
            _tcscpy(fulldisplay, display);
            if (!ParseDisplay(display, tmphost, 255, &_this->m_port)) {
                MessageBox(hwnd, 
                    sz_F8, 
                    sz_F10, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
            } else {
				for (size_t i = 0, len = strlen(tmphost); i < len; i++)
						{
							tmphost[i] = toupper(tmphost[i]);
						} 
                _tcscpy(_this->m_host_dialog, tmphost);
				_this->m_pMRU->AddItem(fulldisplay);
//				_tcscpy(_this->m_remotehost, fulldisplay);
                EndDialog(hwnd, TRUE);
            }

			GetDlgItemText(hwnd, IDC_PROXY_EDIT, display, 256);
            _tcscpy(fulldisplay, display);

			//adzm 2010-02-15
			if (strlen(display) > 0) {
				TCHAR actualProxy[256];
				strcpy(actualProxy, display);

				if (strncmp(tmphost, "ID", 2) == 0) {

					int numericId = _this->m_port;

					int numberOfHosts = 1;
					for (int i = 0; i < (int)strlen(display); i++) {
						if (display[i] == ';') {
							numberOfHosts++;
						}
					}

					if (numberOfHosts <= 1) {
						// then hostname == actualhostname
					} else {
						int modulo = numericId % numberOfHosts;

						char* szToken = strtok(display, ";");
						while (szToken) {
							if (modulo == 0) {
								strcpy(actualProxy, szToken);
								break;
							}

							modulo--;
							szToken = strtok(NULL, ";");
						}
					}
				}

				if (!ParseDisplay(actualProxy, tmphost, 255, &_this->m_proxyport)) {
					MessageBox(hwnd, 
						sz_F8, 
						sz_F10, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
				} else {
					_tcscpy(_this->m_proxyhost, tmphost);
					EndDialog(hwnd, TRUE);
				}
			}

			HWND hProxy = GetDlgItem(hwnd, IDC_PROXY_CHECK);
			if (SendMessage(hProxy, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				_this->m_pOpt->m_fUseProxy = true;
				_this->m_fUseProxy = true;
			}
			else 
			{
				_this->m_pOpt->m_fUseProxy = false;
				_this->m_fUseProxy = false;
			}

			// sf@2002 - DSMPlugin loading
			// If Use plugin is checked, load the plugin if necessary
			HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				TCHAR szPlugin[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
				_this->m_pOpt->m_fUseDSMPlugin = true;
				strcpy(_this->m_pOpt->m_szDSMPluginFilename, szPlugin);

				if (!_this->m_pDSMPlugin->IsLoaded())
				{
					_this->m_pDSMPlugin->LoadPlugin(szPlugin, _this->m_pOpt->m_listening);
					if (_this->m_pDSMPlugin->IsLoaded())
					{
						if (_this->m_pDSMPlugin->InitPlugin())
						{
							if (!_this->m_pDSMPlugin->SupportsMultithreaded())
								_this->m_pOpt->m_oldplugin=true; //PGM
							else //PGM
								_this->m_pOpt->m_oldplugin=false;

							_this->m_pDSMPlugin->SetEnabled(true);
							_this->m_pDSMPlugin->DescribePlugin();
						}
						else
						{
							_this->m_pDSMPlugin->SetEnabled(false);
							MessageBox(hwnd, 
							sz_F7, 
							sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
							return TRUE;
						}
					}
					else
					{
						_this->m_pDSMPlugin->SetEnabled(false);
						MessageBox(hwnd, 
							sz_F5, 
							sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
						return TRUE;
					}
				}
				else
				{
					// sf@2003 - If the plugin is already loaded here it has been loaded
					// by clicking on the config button: we need to init it !
					// But we must first check that the loaded plugin is the same that 
					// the one currently selected...
					_this->m_pDSMPlugin->DescribePlugin();
					if (_stricmp(_this->m_pDSMPlugin->GetPluginFileName(), szPlugin))
					{
						// Unload the previous plugin
						_this->m_pDSMPlugin->UnloadPlugin();
						// Load the new selected one
						_this->m_pDSMPlugin->LoadPlugin(szPlugin, _this->m_pOpt->m_listening);
					}

					if (_this->m_pDSMPlugin->IsLoaded())
					{
						if (_this->m_pDSMPlugin->InitPlugin())
						{
							if (!_this->m_pDSMPlugin->SupportsMultithreaded())
								_this->m_pOpt->m_oldplugin=true; //PGM
							else //PGM
								_this->m_pOpt->m_oldplugin=false;
							_this->m_pDSMPlugin->SetEnabled(true);
							_this->m_pDSMPlugin->DescribePlugin();
						}
						else
						{
							_this->m_pDSMPlugin->SetEnabled(false);
							MessageBox(hwnd, 
							sz_F7, 
							sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
							return TRUE;
						}
					}
					else
					{
						_this->m_pDSMPlugin->SetEnabled(false);
						MessageBox(hwnd, 
							sz_F5, 
							sz_F6, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
						return TRUE;
					}
				}
			}
			else // If Use plugin unchecked but the plugin is loaded, unload it
			{
				_this->m_pOpt->m_fUseDSMPlugin = false;
				if (_this->m_pDSMPlugin->IsEnabled())
				{
					_this->m_pDSMPlugin->UnloadPlugin();
					_this->m_pDSMPlugin->SetEnabled(false);
				}
			}

			if (_this->m_fFromOptions || _this->m_fFromFile)
			{
				EndDialog(hwnd, TRUE);
				return TRUE;
			}

			// sf@2002 - Quick options handling
			_this->ManageQuickOptions(_this, hwnd);

			HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY_CHECK);
			_this->m_pOpt->m_ViewOnly = (SendMessage(hViewOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hAutoScaling = GetDlgItem(hwnd, IDC_AUTOSCALING_CHECK);
			_this->m_pOpt->m_fAutoScaling = (SendMessage(hAutoScaling, BM_GETCHECK, 0, 0) == BST_CHECKED);

			HWND hExitCheck = GetDlgItem(hwnd, IDC_EXIT_CHECK); //PGM @ Advantig
			_this->m_pOpt->m_fExitCheck = (SendMessage(hExitCheck, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM @ Advantig

			EndDialog(hwnd, TRUE);
			return TRUE;
			}

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;

		case IDC_OPTIONBUTTON:
			{
				if (!_this->m_fFromFile)
				{
					_this->ManageQuickOptions(_this, hwnd);					

					HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY_CHECK);
					_this->m_pOpt->m_ViewOnly = (SendMessage(hViewOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);
					
					HWND hAutoScaling = GetDlgItem(hwnd, IDC_AUTOSCALING_CHECK);
					_this->m_pOpt->m_fAutoScaling = (SendMessage(hAutoScaling, BM_GETCHECK, 0, 0) == BST_CHECKED);

					HWND hExitCheck = GetDlgItem(hwnd, IDC_EXIT_CHECK); //PGM @ Advantig
					_this->m_pOpt->m_fExitCheck = (SendMessage(hExitCheck, BM_GETCHECK, 0, 0) == BST_CHECKED); //PGM @ Advantig

				}
				
				if (_this->m_pOpt->DoDialog())
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
				_this->m_pOpt->m_quickoption = 8;
				}
				return TRUE;
			}

		// sf@2002 
		case IDC_PLUGIN_CHECK:
			{
				HWND hUse = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
				BOOL enable = SendMessage(hUse, BM_GETCHECK, 0, 0) == BST_CHECKED;
				HWND hButton = GetDlgItem(hwnd, IDC_PLUGIN_BUTTON);
				EnableWindow(hButton, enable);
			}
			return TRUE;


		case IDC_PLUGIN_BUTTON:
			{
			HWND hPlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
			if (SendMessage(hPlugin, BM_GETCHECK, 0, 0) == BST_CHECKED)
			{
				TCHAR szPlugin[MAX_PATH];
				GetDlgItemText(hwnd, IDC_PLUGINS_COMBO, szPlugin, MAX_PATH);
				// sf@2003 - The config button can be clicked several times with 
				// different selected plugins...
				bool fLoadIt = true;
				char szParams[64];
				strcpy(szParams, sz_F4);
				// If a plugin is already loaded, check if it is the same that the one 
				// we want to load.
				if (_this->m_pDSMPlugin->IsLoaded())
				{
					_this->m_pDSMPlugin->DescribePlugin();
					if (!_stricmp(_this->m_pDSMPlugin->GetPluginFileName(), szPlugin))
					{
						fLoadIt = false;
						_this->m_pDSMPlugin->SetPluginParams(hwnd, szParams);
					}
					else
					{
						// Unload the previous plugin
						_this->m_pDSMPlugin->UnloadPlugin();
						fLoadIt = true;
					}
				}

				if (!fLoadIt) return TRUE;

				if (_this->m_pDSMPlugin->LoadPlugin(szPlugin, _this->m_pOpt->m_listening))
				{
					// We don't know the password yet... no matter the plugin requires
					// it or not, we will provide it later (at plugin "real" startup)
					// Knowing the environnement ("viewer") right now can be usefull or
					// even mandatory for the plugin (specific params saving and so on...)
					// The plugin receives environnement info but isn't inited at this point
					_this->m_pDSMPlugin->SetPluginParams(hwnd, szParams);
				}
				else
				{
					MessageBox(hwnd, 
						sz_F1, 
						sz_F3, MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_TOPMOST);
				}
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
						_tcscpy(szHost, _this->m_pCC->m_host);
					else if (_this->m_pCC->m_port > 5900 && _this->m_pCC->m_port <= 5999)
						_stprintf(szHost, TEXT("%s:%d"), _this->m_pCC->m_host, _this->m_pCC->m_port - 5900);
					else
						_stprintf(szHost, TEXT("%s::%d"), _this->m_pCC->m_host, _this->m_pCC->m_port);

					SetDlgItemText(hwnd, IDC_HOSTNAME_EDIT, szHost);
					//AaronP
					HWND hPlugins = GetDlgItem(hwnd, IDC_PLUGINS_COMBO);
					if( strcmp( _this->m_pOpt->m_szDSMPluginFilename, "" ) != 0 && _this->m_pOpt->m_fUseDSMPlugin ) { 
						int pos = SendMessage(hPlugins, CB_FINDSTRINGEXACT, -1,
							(LPARAM)&(_this->m_pOpt->m_szDSMPluginFilename[0]));

						if( pos != CB_ERR ) {
							SendMessage(hPlugins, CB_SETCURSEL, pos, 0);
							HWND hUsePlugin = GetDlgItem(hwnd, IDC_PLUGIN_CHECK);
							SendMessage(hUsePlugin, BM_SETCHECK, TRUE, 0);
						}
					}
					//EndAaronP
				}
				SetFocus(GetDlgItem(hwnd, IDC_HOSTNAME_EDIT));
 				_this->SetQuickOption(_this, hwnd);

   		        HWND hViewOnly = GetDlgItem(hwnd, IDC_VIEWONLY_CHECK);
		        SendMessage(hViewOnly, BM_SETCHECK, _this->m_pOpt->m_ViewOnly, 0);

				HWND hAutoScaling = GetDlgItem(hwnd, IDC_AUTOSCALING_CHECK);
				SendMessage(hAutoScaling, BM_SETCHECK, _this->m_pOpt->m_fAutoScaling, 0);

				HWND hExitCheck = GetDlgItem(hwnd, IDC_EXIT_CHECK); //PGM @ Advantig
				SendMessage(hExitCheck, BM_SETCHECK, _this->m_pOpt->m_fExitCheck, 0); //PGM @ Advantig

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
		EndDialog(hwnd, FALSE);
		return FALSE;
	case WM_DESTROY:
		EndDialog(hwnd, FALSE);
		return TRUE;
	}
	return 0;
}


//
//  Set the Quick Option Radio button, depending on the current QuickOption
//
int SessionDialog::SetQuickOption(SessionDialog* p_SD, HWND hwnd)
{
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
	SendMessage(hManual, BM_SETCHECK, false, 0);

	// sf@2002 - Select Modem Option as default
	switch (p_SD->m_pOpt->m_quickoption)
	{
	case 1: // AUTO
		{
		HWND hDyn = GetDlgItem(hwnd, IDC_DYNAMIC);
		SendMessage(hDyn, BM_SETCHECK, true, 0);
		}
		break;

	case 2: // LAN
		{
		HWND hLan = GetDlgItem(hwnd, IDC_LAN_RB);
		SendMessage(hLan, BM_SETCHECK, true, 0);
		}
		break;
	case 3: // MEDIUM
		{
		HWND hMedium = GetDlgItem(hwnd, IDC_MEDIUM_RB);
		SendMessage(hMedium, BM_SETCHECK, true, 0);
		}
		break;
	case 4: // MODEM
		{
		HWND hModem = GetDlgItem(hwnd, IDC_MODEM_RB);
		SendMessage(hModem, BM_SETCHECK, true, 0);
		}
		break;
	case 5: // SLOW
		{
		HWND hSlow = GetDlgItem(hwnd, IDC_SLOW_RB);
		SendMessage(hSlow, BM_SETCHECK, true, 0);
		}
		break;
	case 7: // LAN
		{
		HWND hUltraLan = GetDlgItem(hwnd, IDC_ULTRA_LAN_RB);
		SendMessage(hUltraLan, BM_SETCHECK, true, 0);
		}
		break;
	default: // MANUAL
		{
		HWND hManual = GetDlgItem(hwnd, IDC_MANUAL);
		SendMessage(hManual, BM_SETCHECK, true, 0);
		}
		break;
	/*
	case 6: // BUSY
		{
		HWND hBusy = GetDlgItem(hwnd, IDC_BUSY_RB);
		SendMessage(hBusy, BM_SETCHECK, true, 0);
		}
		break;
	*/
	}
	return 0;
}


//
//  Get the selected Quick Option and set the parameters consequently
// 
int SessionDialog::ManageQuickOptions(SessionDialog* _this, HWND hwnd)
{
	// Auto Mode
	HWND hDynamic = GetDlgItem(hwnd, IDC_DYNAMIC);
	if ((SendMessage(hDynamic, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 1;

	// Options for LAN Mode
	HWND hUltraLan = GetDlgItem(hwnd, IDC_ULTRA_LAN_RB);
	if ((SendMessage(hUltraLan, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 7;

	// Options for LAN Mode
	HWND hLan = GetDlgItem(hwnd, IDC_LAN_RB);
	if ((SendMessage(hLan, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 2;

	// Options for Medium Mode
	HWND hMedium = GetDlgItem(hwnd, IDC_MEDIUM_RB);
	if ((SendMessage(hMedium, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 3;

	// Options for Modem Mode 
	HWND hModem = GetDlgItem(hwnd, IDC_MODEM_RB);
	if ((SendMessage(hModem, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 4;

	// Options for Slow Mode
	HWND hLow = GetDlgItem(hwnd, IDC_SLOW_RB);
	if ((SendMessage(hLow, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 5;

	// Options for Manual
	HWND hManual = GetDlgItem(hwnd, IDC_MANUAL);
	if ((SendMessage(hManual, BM_GETCHECK, 0, 0) == BST_CHECKED))
		_this->m_pOpt->m_quickoption = 8;

	// Set the params depending on the selected QuickOption
	_this->m_pCC->HandleQuickOption();

	return 0;
}