//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.

// WinVNC.cpp

// 24/11/97		WEZ

// WinMain and main WndProc for the new version of WinVNC

////////////////////////////
// System headers
#include "stdhdrs.h"

////////////////////////////
// Custom headers
#include "VSocket.h"
#include "WinVNC.h"

#include "vncServer.h"
#include "vncMenu.h"
#include "vncInstHandler.h"
#include "vncService.h"
///unload driver
#include "vncOSVersion.h"
#include "videodriver.h"
//#define CRASH_ENABLED
#ifdef CRASH_ENABLED
	#ifndef _CRASH_RPT_
	#include "crashrpt.h"
	#pragma comment(lib, "crashrpt.lib")
	#endif
#endif
#include <commctrl.h>
#define LOCALIZATION_MESSAGES   // ACT: full declaration instead on extern ones
#include "localization.h" // Act : add localization on messages
#ifdef SOUND
DWORD WINAPI sound_Threadproc(LPVOID lpParameter);
extern bool sound;
#endif




// Application instance and name
HINSTANCE	hAppInstance;
const char	*szAppName = "WinVNC";
DWORD		mainthreadId;
BOOL		AllowMulti=false;
BOOL		DisableMultiWarning=false;
BOOL		g_plugin=false;
char		g_var[25][150];
char		g_var_20[25][50];
char		g_idcode[_MAX_PATH]="";
bool		g_id=false;
char		TextTop[150]="";
char		TextMiddle[150]="";
char		TextBottom[150]="";
char		TextRTop[150]="";
char		TextRBottom[150]="";
char		TextButton[150]="";
char		TextCButton[150]="";
char		TextRMiddle[150]="";
char		TextTitle[150]="";
char		Webpage[150]="";
bool		bWebpage;
char		Balloon1Title[150]="";
char		Balloon2Title[150]="";
char		Balloon1A[150]="";
char		Balloon1B[150]="";
char		Balloon1C[150]="";
char		Balloon2A[150]="";
char		Balloon2B[150]="";
char		Balloon2C[150]="";

#ifdef SINGLECLICKULTRA
// actual default command line to be patched after compilation
char  defaultCommandLine[1024]="";
#endif


static HWND hList=NULL;  // List View identifier
LVCOLUMN LvCol; // Make Coluom struct for ListView
LVITEM LvItem;  // ListView Item struct
int iSelect=0;
int flag=0;
HWND hEdit;
HBITMAP DoGetBkGndBitmap(IN CONST UINT uBmpResId);
BOOL DoSDKEraseBkGnd(IN CONST HDC hDC,IN CONST COLORREF crBkGndFill);
int m_mytimerid;
    

BOOL CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
//	if ((Message!=WM_TIMER || Message!=WM_CLOSE) && scanner) return false;
  switch(Message)
  {
	    	case WM_ERASEBKGND:
            {
                //DoSDKEraseBkGnd((HDC)wParam, RGB(255,0,0));
				return DoSDKEraseBkGnd((HDC)wParam, RGB(255,0,0));
            }
			case WM_CTLCOLORSTATIC:
			case WM_CTLCOLOREDIT:
			{
				SetBkMode((HDC) wParam, TRANSPARENT);
				return (DWORD) GetStockObject(NULL_BRUSH);
			}
        
         // This Window Message will close the dialog  //
		//============================================//
		case WM_CLOSE:
			{
			     EndDialog(hWnd,0); // kill dialog
				 PostMessage(hWnd, WM_QUIT, 0, 0);
				 exit(0);
			}
			break;

		case WM_NOTIFY:
		{
			switch(LOWORD(wParam))
			{
			    case IDC_LIST: 
                if(((LPNMHDR)lParam)->code == NM_DBLCLK)
				{
				  char Text[255]={0};  
				  char Temp[255]={0};
				  char Temp1[255]={0};
				  int iSlected=0;
				  int j=0;

				  iSlected=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
				  
				  if(iSlected==-1)
				  {
                    MessageBox(hWnd,"No Items in ListView","Error",MB_OK|MB_ICONINFORMATION);
					break;
				  }

				  memset(&LvItem,0,sizeof(LvItem));
                  LvItem.mask=LVIF_TEXT;
				  LvItem.iSubItem=0;
				  LvItem.pszText=Text;
				  LvItem.cchTextMax=256;
				  LvItem.iItem=iSlected;
                  
				  SendMessage(hList,LVM_GETITEMTEXT, iSlected, (LPARAM)&LvItem);
				  
				  sprintf(Temp1,Text);
				  sprintf(Temp1,g_var[iSlected]);
				  strcpy(defaultCommandLine,Temp1);
				  EndDialog(hWnd,0);
				  
				  /*for(j=1;j<=4;j++)
				  {
					LvItem.iSubItem=j;
				    SendMessage(hList,LVM_GETITEMTEXT, iSlected, (LPARAM)&LvItem);
				    sprintf(Temp," %s",Text);
					lstrcat(Temp1,Temp);
				  }*/

//				  startvnc(Temp1);

				}
				if(((LPNMHDR)lParam)->code == NM_CLICK)
				{
					iSelect=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
				    
					if(iSelect==-1)
					{
                      MessageBox(hWnd,"No Vnc server selected","Error",MB_OK|MB_ICONINFORMATION);
					  break;
					}
					flag=1;
				}

				if(((LPNMHDR)lParam)->code == LVN_BEGINLABELEDIT)
				{
                  hEdit=ListView_GetEditControl(hList);
				}
				
				if(((LPNMHDR)lParam)->code == LVN_ENDLABELEDIT)
				{
					int iIndex;
					char text[255]="";
					iIndex=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
				    LvItem.iSubItem=0;
                    LvItem.pszText=text;
                    GetWindowText(hEdit, text, sizeof(text));
					SendMessage(hList,LVM_SETITEMTEXT,(WPARAM)iIndex,(LPARAM)&LvItem);
				}
				break;
			}
		}

		case WM_PAINT:
			{
				return 0;
			}
			break;
		case WM_TIMER:
			{
				EndDialog(hWnd,0);
				KillTimer(NULL, m_mytimerid);
			}
			break;

		// This Window Message is the heart of the dialog  //
		//================================================//
		case WM_INITDIALOG:
			{
//				GetIp();

                SetFocus(hWnd);
				hList=GetDlgItem(hWnd,IDC_LIST); // get the ID of the ListView
				SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT); // Set style

				// Here we put the info on the Coulom headers
				// this is not data, only name of each header we like
                memset(&LvCol,0,sizeof(LvCol)); // Reset Coluom
				LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM; // Type of mask
				LvCol.cx=0x100;                                // width between each coloum
				LvCol.pszText="Select Helpdesk to connect to .......";                     // First Header
// 				LvCol.cx=0x60;

				// Inserting Couloms as much as we want
				SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); // Insert/Show the coloum
                memset(&LvItem,0,sizeof(LvItem)); // Reset Item Struct
				
				//  Setting properties Of Items:

				LvItem.mask=LVIF_TEXT;   // Text Style
				LvItem.cchTextMax = 256; // Max size of test
                
				LvItem.iItem=0;          // choose item  
				LvItem.iSubItem=0;       // Put in first coluom
				LvItem.pszText="Item 0"; // Text to display (can be from a char variable) (Items)
//				m_timer=SetTimer( hWnd, 1,  200, NULL);

				FILE *fid;
				bool done=false;
				char menustrings[1024];
				int iItem;
//				char ItemText[160];
				if (GetModuleFileName(NULL, menustrings, 1024))
					{
						char* p = strrchr(menustrings, '\\');
						if (p == NULL) return 0;
						*p = '\0';
						strcat (menustrings,"\\helpdesk.txt");
					}
	
				int i=0;
				bool direct=false;
				if ((fid = fopen(menustrings, "r"))!=NULL)
					{
					    HBITMAP hImage = (HBITMAP)LoadImage(NULL, "logo.bmp", IMAGE_BITMAP, 0, 0,LR_LOADFROMFILE); 
						HWND logo=GetDlgItem(hWnd, IDC_LOGO);
						if (hImage) SendMessage(logo, STM_SETIMAGE,(WPARAM)IMAGE_BITMAP,(LPARAM)(HANDLE)hImage);
						char myline[150];
						bWebpage=false;
						while( fgets( myline, sizeof(myline), fid ))
							{
								int j=0;
								if (strncmp(myline, "[END]", strlen("[END]")) == 0) {
								break;
								}
								if (strncmp(myline, "[BEGIN HOSTLIST]", strlen("[BEGIN HOSTLIST]")) == 0) {
								continue;
								}


								if (strncmp(myline, "[HOST]", strlen("[HOST]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(g_var_20[i],"");
								strncpy(g_var_20[i],myline,strlen(myline)-1);
								iItem=SendMessage(hList,LVM_GETITEMCOUNT,0,0);
								LvItem.iItem=iItem;            // choose item  
								LvItem.iSubItem=0;         // Put in first coluom
								LvItem.pszText=g_var_20[i];
								SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
								fgets( myline, sizeof(myline), fid );
								strcpy(g_var[i],"");
								strcpy(g_var[i],myline);
								i++;
								}

								if (strncmp(myline, "[TEXTTOP]", strlen("[TEXTTOP]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextTop,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_TEXTTOP, WM_SETTEXT, 0, (LONG)TextTop);
								}

								if (strncmp(myline, "[TEXTMIDDLE]", strlen("[TEXTMIDDLE]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextMiddle,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_TEXTMIDDLE, WM_SETTEXT, 0, (LONG)TextMiddle);
								}

								if (strncmp(myline, "[TEXTBOTTOM]", strlen("[TEXTBOTTOM]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextBottom,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_TEXTBOTTOM, WM_SETTEXT, 0, (LONG)TextBottom);
								}

								if (strncmp(myline, "[TEXTRTOP]", strlen("[TEXTRTOP]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextRTop,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_TEXTRICHTTOP, WM_SETTEXT, 0, (LONG)TextRTop);
								}

								if (strncmp(myline, "[TEXTRBOTTOM]", strlen("[TEXTRBOTTOM]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextRBottom,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_TEXTRIGHTBOTTOM, WM_SETTEXT, 0, (LONG)TextRBottom);
								}

								if (strncmp(myline, "[TEXTRMIDDLE]", strlen("[TEXTRMIDDLE]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextRMiddle,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_TEXTRIGHTMIDDLE, WM_SETTEXT, 0, (LONG)TextRMiddle);
								}

								if (strncmp(myline, "[TEXTBUTTON]", strlen("[TEXTBUTTON]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strncpy(TextButton,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_HELPWEB, WM_SETTEXT, 0, (LONG)TextButton);
								}

								if (strncmp(myline, "[TEXTCLOSEBUTTON]", strlen("[TEXTCLOSEBUTTON]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(TextCButton,"");
								strncpy(TextCButton,myline,strlen(myline)-1);
								SendDlgItemMessage(hWnd,IDC_CLOSE, WM_SETTEXT, 0, (LONG)TextCButton);
								}

								if (strncmp(myline, "[TITLE]", strlen("[TITLE]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(TextTitle,myline);
								TextTitle[strlen(myline)-1]='\0';
								SetWindowText(hWnd,TextTitle);
								}

								if (strncmp(myline, "[WEBPAGE]", strlen("[WEBPAGE]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Webpage,myline);
								bWebpage=true;
								}

								if (strncmp(myline, "[BALLOON1TITLE]", strlen("[BALLOON1TITLE]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon1Title,myline);
								Balloon1Title[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[BALLOON1A]", strlen("[BALLOON1A]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon1A,myline);
								Balloon1A[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[BALLOON1B]", strlen("[BALLOON1B]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon1B,myline);
								Balloon1B[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[BALLOON1C]", strlen("[BALLOON1C]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon1C,myline);
								Balloon1C[strlen(myline)-1]='\0';
								}

								if (strncmp(myline, "[BALLOON2TITLE]", strlen("[BALLOON2TITLE]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon2Title,myline);
								Balloon2Title[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[BALLOON2A]", strlen("[BALLOON2A]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon2A,myline);
								Balloon2A[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[BALLOON2B]", strlen("[BALLOON2B]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon2B,myline);
								Balloon2B[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[BALLOON2C]", strlen("[BALLOON2C]")) == 0) {
								fgets( myline, sizeof(myline), fid );
								strcpy(Balloon2C,myline);
								Balloon2C[strlen(myline)-1]='\0';
								}
								if (strncmp(myline, "[DIRECT]", strlen("[DIRECT]")) == 0) {
								direct=true;
								}



								
								
								
							}
						fclose(fid);
					}				
				else EndDialog(hWnd,0);
				if (i==1 && direct) 
				{
					strcpy(defaultCommandLine,g_var[0]);
					m_mytimerid = SetTimer(hWnd, 11, 2000, NULL);
//				    EndDialog(hWnd,0);
				}
				return TRUE; // Always True			
			}
			break;

     // This Window Message will control the dialog  //
	//==============================================//
        case WM_COMMAND:
			switch (LOWORD(wParam))
				{
					case IDC_CLOSE:
						EndDialog(hWnd,0); // kill dialog
						PostMessage(hWnd, WM_QUIT, 0, 0);
						exit(0);
						break;
			case IDC_HELPWEB:
				if (bWebpage)ShellExecute(GetDesktopWindow(), "open", Webpage, "", 0, SW_SHOWNORMAL);
				break;
					}
			break;
    
	    default:return FALSE;
    }

	return TRUE;
}

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	setbuf(stderr, 0);

        //ACT: Load all messages from ressource file
        Load_Localization(hInstance) ;
        //ACT: end

	// Configure the log file, in case one is required
	vnclog.SetFile("WinVNC.log", false);
	//Load command line from extra menu
	{
		    INITCOMMONCONTROLSEX InitCtrls;
			InitCtrls.dwICC = ICC_LISTVIEW_CLASSES|ICC_INTERNET_CLASSES;
			InitCtrls.dwSize = sizeof(INITCOMMONCONTROLSEX);
			BOOL bRet = InitCommonControlsEx(&InitCtrls);
			DialogBoxParam(hInstance, MAKEINTRESOURCE(IDC_DIALOG), NULL, (DLGPROC)DialogProc,0);
	}
#ifdef SOUND
	char systemroot[150];
	{
	GetEnvironmentVariable("SYSTEMROOT", systemroot, 150);
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
								  strcat(systemroot,"\\system32\\regsvr32 /s CCNSMT.dll");
								  break;
		case VER_PLATFORM_WIN32_WINDOWS:
								  strcat(systemroot,"\\system\\regsvr32 /s CCNSMT.dll");
								  break;
	}
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	int aa=CreateProcess(NULL, systemroot, NULL, NULL, TRUE, 0, NULL,NULL, &si, &pi); 
	}
#endif


#ifdef _DEBUG
	{
		// Get current flag
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );

		// Turn on leak-checking bit
		tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

		// Set flag to the new value
		_CrtSetDbgFlag( tmpFlag );
	}
#endif

	// Save the application instance and main thread id
	hAppInstance = hInstance;
	mainthreadId = GetCurrentThreadId();

	// Initialise the VSocket system
	VSocketSystem socksys;
	if (!socksys.Initialised())
	{
		MessageBox(NULL, sz_ID_FAILED_INIT, szAppName, MB_OK);
		return 0;
	}
	vnclog.Print(LL_STATE, VNCLOG("sockets initialised\n"));

#ifdef SINGLECLICKULTRA
	if (strlen(szCmdLine) == 0) {
		szCmdLine = defaultCommandLine;
	}
#endif

	// Make the command-line lowercase and parse it
	int i;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		szCmdLine[i] = tolower(szCmdLine[i]);
	}

	BOOL argfound = FALSE;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		if (szCmdLine[i] <= ' ')
			continue;
		argfound = TRUE;

		// Now check for command-line arguments

		if (strncmp(&szCmdLine[i], winvncId, strlen(winvncId)) == 0) {
			// Add a new client to an existing copy of winvnc
			i+=strlen(winvncId);

			// First, we have to parse the command line to get the filename to use
			int start, end;
			start = i;
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			end = start;
			while (szCmdLine[end] > ' ') end++;
			strcpy(g_idcode,"");
			strncpy(g_idcode,&szCmdLine[start],end-start);
			g_idcode[end-start]='\0';
			i+=(end-start);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncRunServiceHelper, strlen(winvncRunServiceHelper)) == 0)
		{
			// NB : This flag MUST be parsed BEFORE "-service", otherwise it will match
			// the wrong option!  (This code should really be replaced with a simple
			// parser machine and parse-table...)

			// Run the WinVNC Service Helper app
			vncService::PostUserHelperMessage();
			return 0;
		}
		if (strncmp(&szCmdLine[i], winvncRunService, strlen(winvncRunService)) == 0)
		{
			// Run WinVNC as a service
//			return vncService::WinVNCServiceMain();
		}
		if (strncmp(&szCmdLine[i], winvncRunAsUserApp, strlen(winvncRunAsUserApp)) == 0)
		{
			// WinVNC is being run as a user-level program
			return WinVNCAppMain();
		}

		if (strncmp(&szCmdLine[i], winvncAllowMulti, strlen(winvncAllowMulti)) == 0)
		{
			// WinVNC is being run as a user-level program
			AllowMulti=true;
			i+=strlen(winvncAllowMulti);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncplugin, strlen(winvncplugin)) == 0)
		{
			// WinVNC is being run as a user-level program
			g_plugin=true;
			i+=strlen(winvncplugin);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncDisableMultiWarning, strlen(winvncDisableMultiWarning)) == 0)
		{
			// WinVNC is being run as a user-level program
			DisableMultiWarning=true;
			vncService::Set_Fus(1);
			i+=strlen(winvncDisableMultiWarning);
			vnclog.SetFile("WinVNCfus.log", false);

#ifndef SINGLECLICKULTRA
			return WinVNCAppMain();
#endif
			continue;
		}


		if (strncmp(&szCmdLine[i], winvncInstallService, strlen(winvncInstallService)) == 0)
		{
			// Install WinVNC as a service

//			vncService::InstallService();
			i+=strlen(winvncInstallService);
			continue;
		}

		if (strncmp(&szCmdLine[i], winvncInstallServices, strlen(winvncInstallServices)) == 0)
		{
			// Install WinVNC as a service

//			vncService::InstallService(1);
			i+=strlen(winvncInstallServices);
			continue;
		}


		if (strncmp(&szCmdLine[i], winvncReinstallService, strlen(winvncReinstallService)) == 0)
		{
			// Silently remove WinVNC, then re-install it
//			vncService::ReinstallService();
			i+=strlen(winvncReinstallService);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncRemoveService, strlen(winvncRemoveService)) == 0)
		{
			// Remove the WinVNC service
//			vncService::RemoveService();
			i+=strlen(winvncRemoveService);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncShowProperties, strlen(winvncShowProperties)) == 0)
		{
			// Show the Properties dialog of an existing instance of WinVNC
			vncService::ShowProperties();
			i+=strlen(winvncShowProperties);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncShowDefaultProperties, strlen(winvncShowDefaultProperties)) == 0)
		{
			// Show the Properties dialog of an existing instance of WinVNC
			vncService::ShowDefaultProperties();
			i+=strlen(winvncShowDefaultProperties);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncShowAbout, strlen(winvncShowAbout)) == 0)
		{
			// Show the About dialog of an existing instance of WinVNC
			vncService::ShowAboutBox();
			i+=strlen(winvncShowAbout);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncKillRunningCopy, strlen(winvncKillRunningCopy)) == 0)
		{
			// Kill any already running copy of WinVNC
			vncService::KillRunningCopy();
			i+=strlen(winvncKillRunningCopy);
			continue;
		}
		if (strncmp(&szCmdLine[i], winvncAutoReconnect, strlen(winvncAutoReconnect)) == 0)
		{
			// Note that this "autoreconnect" param MUST be BEFORE the "connect" one
			// on the command line !
			vncService::PostAddNewClient(999, 999); // sf@2003 - I hate to do that ;)
			i+=strlen(winvncAutoReconnect);
			continue;
		}
		/*
		if (strncmp(&szCmdLine[i], winvncFTNoUserImpersonation, strlen(winvncFTNoUserImpersonation)) == 0)
		{
			// We disable User Impersonnation for FT thread
			// (so even a non loged user can access the whole FileSystem)
			vncService::PostAddNewClient(998, 998); // sf@2005 - I still hate to do that ;)
			i+=strlen(winvncFTNoUserImpersonation);
			continue;
		}
		*/
		if (strncmp(&szCmdLine[i], winvncAddNewClient, strlen(winvncAddNewClient)) == 0)
		{
			// Add a new client to an existing copy of winvnc
			i+=strlen(winvncAddNewClient);

			// First, we have to parse the command line to get the filename to use
			int start, end;
			start=i;
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			end = start;
			while (szCmdLine[end] > ' ') end++;

			// Was there a hostname (and optionally a port number) given?
			if (end-start > 0)
			{
				char *name = new char[end-start+1];
				if (name != 0) {
					strncpy(name, &(szCmdLine[start]), end-start);
					name[end-start] = 0;

					int port = INCOMING_PORT_OFFSET;
					char *portp = strchr(name, ':');
					if (portp) {
						*portp++ = '\0';
						if (*portp == ':') {
							port = atoi(++portp);	// Port number after "::"
						} else {
							port = atoi(portp);	// Display number after ":"
						}
					}
					char test[150];
					strcpy(test,name);
					vnclog.Print(LL_STATE, VNCLOG("test... %s %d\n"),name,port);
					VCard32 address = VSocket::Resolve(name);
					delete [] name;
					if (address != 0) {
						// Post the IP address to the server

#ifndef SINGLECLICKULTRA
						vncService::PostAddNewClient(address, port);
#else
						//vncService::PostAddNewClient(address, port);
						if (!vncService::PostAddNewClient(address, port)) {
							// VNC isn't running so we will start it now
							return WinVNCAppMain(address, port);
						}
#endif
					}
				}
				i=end;
				continue;
			}
			else 
			{
				// Tell the server to show the Add New Client dialog
				vncService::PostAddNewClient(0, 0);
			}
			continue;
		}

		// Either the user gave the -help option or there is something odd on the cmd-line!

		// Show the usage dialog
		MessageBox(NULL, defaultCommandLine, sz_ID_WINVNC_USAGE, MB_OK | MB_ICONINFORMATION);
		break;
	};

	// If no arguments were given then just run
	if (!argfound)
		return WinVNCAppMain();

	return 0;
}

// This is the main routine for WinVNC when running as an application
// (under Windows 95 or Windows NT)
// Under NT, WinVNC can also run as a service.  The WinVNCServerMain routine,
// defined in the vncService header, is used instead when running as a service.


#ifndef SINGLECLICKULTRA

int WinVNCAppMain()
#else

int WinVNCAppMain(unsigned long ipaddress, unsigned short port)
#endif
{
#ifdef CRASH_ENABLED
	LPVOID lpvState = Install(NULL,  "rudi.de.vos@skynet.be", "UltraVnc v100 RC12G");
#endif
	// Set this process to be the last application to be shut down.
	// Check for previous instances of WinVNC!
	vncInstHandler *instancehan=new vncInstHandler;
	
	if (!instancehan->Init())
	{
		if (!AllowMulti)
		{
			// We don't allow multiple instances!
			if (!DisableMultiWarning) MessageBox(NULL, sz_ID_ANOTHER_INST, szAppName, MB_OK);
			return 0;
		}
	}

	// CREATE SERVER
	vncServer server;

	// Set the name and port number
	server.SetName(szAppName);
	vnclog.Print(LL_STATE, VNCLOG("server created ok\n"));
	///uninstall driver before cont
	
#ifdef SINGLECLICKULTRA
	char* szCmdLine = defaultCommandLine;
	// Make the command-line lowercase and parse it
	int i;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		szCmdLine[i] = tolower(szCmdLine[i]);
	}

	BOOL argfound = FALSE;
	for (i = 0; i < strlen(szCmdLine); i++)
	{
		if (szCmdLine[i] <= ' ')
			continue;
		argfound = TRUE;
		if (strncmp(&szCmdLine[i], winvncPassword, strlen(winvncPassword)) == 0) {
			// Add a new client to an existing copy of winvnc
			i+=strlen(winvncPassword);

			// First, we have to parse the command line to get the filename to use
			int start, end;
			start = i;
			while (szCmdLine[start] <= ' ' && szCmdLine[start] != 0) start++;
			end = start;
			while (szCmdLine[end] > ' ') end++;
			szCmdLine[end] = '\0';
//			server.SetPassword(&szCmdLine[start]);
			i = end;
			continue;
		}
		
		if (strncmp(&szCmdLine[i], winvncNoRegistry, strlen(winvncNoRegistry)) == 0) {
			i+=strlen(winvncNoRegistry);
//			server.SetAllowRegistry(FALSE);
		}
		if (strncmp(&szCmdLine[i], winvncNoTrayicon, strlen(winvncNoTrayicon)) == 0) {
			i+=strlen(winvncNoRegistry);
			server.SetDisableTrayIcon(TRUE);
		}
		if (strncmp(&szCmdLine[i], winvncReadonly, strlen(winvncReadonly)) == 0) {
			i+=strlen(winvncReadonly);
			server.EnableRemoteInputs(FALSE);
		}
		if (strncmp(&szCmdLine[i], winvncUsername, strlen(winvncUsername)) == 0) {
			i+=strlen(winvncUsername);
			TCHAR username[UNLEN + 1];
			DWORD usernamelen;
			usernamelen = sizeof(username);
			if (GetUserName(username, &usernamelen)) {
				server.SetName(username);
			}
		}
	}
#endif

	


	

	// Create tray icon & menu if we're running as an app
	vncMenu *menu = new vncMenu(&server);
	if (menu == NULL)
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to create tray menu\n"));
		PostQuitMessage(0);
	}

#ifdef SINGLECLICKULTRA
	// If Address and Port are specified then Call AddNewClient
	if (ipaddress > 0 && port > 0)
		vncService::PostAddNewClient(ipaddress, port);

#endif
	// Now enter the message handling loop until told to quit!
	MSG msg;
	while (GetMessage(&msg, NULL, 0,0) ) {
		//vnclog.Print(LL_INTINFO, VNCLOG("Message %d received\n"), msg.message);

		TranslateMessage(&msg);  // convert key ups and downs to chars
		DispatchMessage(&msg);
	}
	vnclog.Print(LL_STATE, VNCLOG("shutting down server\n"));
#ifdef SOUND
	{
	char systemroot[150];
	GetEnvironmentVariable("SYSTEMROOT", systemroot, 150);
	OSVERSIONINFO OSversion;
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
	GetVersionEx(&OSversion);
	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
								  strcat(systemroot,"\\system32\\regsvr32 /u /s CCNSMT.dll");
								  break;
		case VER_PLATFORM_WIN32_WINDOWS:
								  strcat(systemroot,"\\system\\regsvr32 /u /s CCNSMT.dll");
								  break;
	}
	sound=false;
	Sleep(1100);
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	int aa=CreateProcess(NULL, systemroot, NULL, NULL, TRUE, 0, NULL,NULL, &si, &pi); 

	}
#endif

	if (menu != NULL)
		delete menu;
	if (instancehan!=NULL)
		delete instancehan;

	return msg.wParam;
};
