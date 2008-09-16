
#include "stdhdrs.h"
#include "vncviewer.h"
#include "FullScreenTitleBar.h"
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#include "AboutBox.h"
#include <Windowsx.h>
#include "HotkeyDlg.h"

extern char sz_L2[64];
extern char sz_L3[64];
extern char sz_L4[64];
extern char sz_L5[64];
extern char sz_L6[64];
extern char sz_L7[64];
extern char sz_L8[64];
extern char sz_L9[64];
extern char sz_L10[64];
extern char sz_L11[64];
extern char sz_L12[64];
extern char sz_L13[64];
extern char sz_L14[64];
extern char sz_L15[64];

extern int g_iMaxChild;
extern int g_iNumChild;
extern CHILD* g_child;

extern HINSTANCE m_hInstResDLL;
HWND m_hwndMain=NULL;

HWND m_TrafficMonitor=NULL;
HWND m_hMDIClient=NULL;
HWND m_Status=NULL;
HWND m_hwndTab=NULL;
HWND hwndStatic=NULL;
HWND hWndToolBar =NULL;
HWND hWndRebar = NULL;
HWND hwndCombo=NULL;

HANDLE m_bitmapFRONT,m_bitmapBACK,m_bitmapNONE;
HMENU menu;
bool fullscreen;
CTitleBar TitleBar;
int g_iMaxChild = 50;                          // Maximum number of child windows wanted
int g_iNumChild = 0;                           // Number of child windows created
CHILD* g_child = new CHILD[g_iMaxChild];       // Create the CHILD structures
VNCviewerApp *app;
bool g_stop=false;
bool Zoomed=false;

int g_cyToolBar;	 // the height of toolbar
int g_cyToolBar0;  // the height of toolbar
int g_cyStatusBar; // the height of status bar
int g_cyStatusBar0; // the height of status bar
bool f_StatusBar=true;
bool f_ToolBar=true;
bool g_saved_open=false;
bool g_autoscale=true;
char ini[MAX_PATH] = "";

TCHAR g_FileName[MAX_PATH];


#define VWR_WND_CLASS_NAME _T("VNCviewer")
#define PARENT_CLASS_NAME _T("VNCviewerParent")
#define ID_MDI_FIRSTCHILD 60000

//
// Process windows messages
//

#include <commctrl.h>
#include <shellapi.h>
#include <lmaccess.h>
#include <lmat.h>
#include <lmalert.h>

static void MoveRebar( void );
static void ToolBar( HWND hWndParent );
static void CreateStatusWindow();
static void CreateTrafficWindow();
void CreateDisplay();
BOOL ResizeAllWindows( void );
void SetupHotkey(char* name, int id,char* ini);

void
FillCombo(HWND m_hcombo)
{
	HANDLE fileh;
	WIN32_FIND_DATA finddata;
	char WORKDIR[MAX_PATH];
	char FULLFILL[MAX_PATH];
	char FILENAME[MAX_PATH];
	TCHAR szString[MAX_PATH];
	bool is;
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
				{
				char* p = strrchr(WORKDIR, '\\');
					if (p == NULL) return;
					*p = '\0';
				}
	strcpy(FULLFILL,"");
	strcat(FULLFILL,WORKDIR);//set the directory
	strcat(FULLFILL,"\\viewers\\*.vnc");//set the filter
	fileh=FindFirstFile(FULLFILL,&finddata);
	if (fileh == INVALID_HANDLE_VALUE) 
	{
	}
	else
	{
			SendMessage (m_hcombo, CB_RESETCONTENT, 0, (LPARAM)finddata.cFileName);
			SendMessage (m_hcombo, CB_ADDSTRING, 0, (LPARAM)finddata.cFileName);
	is=FindNextFile(fileh,&finddata);
	while(is)
		{
		if(finddata.dwFileAttributes!=FILE_ATTRIBUTE_DIRECTORY )//we dont want any directory listings do we? 
	        {
				if(is)
					SendMessage (m_hcombo, CB_ADDSTRING, 0, (LPARAM)finddata.cFileName);
			}
		is=FindNextFile(fileh,&finddata);
		}
    SendMessage (m_hcombo, CB_SETCURSEL, (WPARAM)0, 0);
	FindClose(fileh);
	}
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	
			HWND hwnd_active_child	;
			hwnd_active_child = (HWND) SendMessage(m_hMDIClient, WM_MDIGETACTIVE, 0, 0);
			switch (iMsg)
			{
			case WM_CREATE:
				if (f_ToolBar) CheckMenuItem(menu,ID_SETTINGS_BUTTONSON,MF_CHECKED);
				else CheckMenuItem(menu,ID_SETTINGS_BUTTONSON,MF_UNCHECKED);
				if (f_StatusBar) CheckMenuItem(menu,ID_SETTINGS_TABBAR,MF_CHECKED);
				else CheckMenuItem(menu,ID_SETTINGS_TABBAR,MF_UNCHECKED);
				if (g_saved_open) CheckMenuItem(menu,ID_SETTINGS_SAVESESSIONONEXITON,MF_CHECKED);
				else CheckMenuItem(menu,ID_SETTINGS_SAVESESSIONONEXITON,MF_UNCHECKED);
				if (g_autoscale) CheckMenuItem(menu,ID_SETTINGS_AUTORESIZEON,MF_CHECKED);
				else CheckMenuItem(menu,ID_SETTINGS_AUTORESIZEON,MF_UNCHECKED);
				
				break;
			
			case WM_SYSCOMMAND:
				{
					switch (LOWORD(wParam))
					{
					case IDD_STOPLISTENER:
						app->StopListener();
						return 0;
					default:
						break;
					}

				}//end case wm_syscommand
			case WM_COMMAND:
						{
							switch(LOWORD(wParam))
							{
							case ID_BUTTONSELECT:
								{
//								 mystruct mys;
								 char filename[MAX_PATH];
								 char WORKDIR[MAX_PATH];
								 TCHAR FULLFILL[MAX_PATH];
								 int i=SendMessage(hwndCombo,CB_GETCURSEL,0,0);
								 if (i!=CB_ERR)
								 {
									int Length=SendMessage(hwndCombo,CB_GETLBTEXTLEN,i, 0)+1;
									if (Length<MAX_PATH) 
										{
											SendMessage(hwndCombo, CB_GETLBTEXT, i, (LPARAM)filename) ;
											if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
												{
													char* p = strrchr(WORKDIR, '\\');
													if (p == NULL) return 0;
													*p = '\0';
												}
											strcpy(FULLFILL,"");
											strcat(FULLFILL,WORKDIR);//set the directory
											strcat(FULLFILL,"\\viewers\\");
											strcat(FULLFILL,filename);//set the filter
											strcpy(g_FileName,FULLFILL);

											SendMessage(m_hwndMain,WM_COMMAND,ID_NEWCONNF,0);
											//app->NewConnection(FULLFILL);
										}
//									Beep(1000,100);
									break;
								 }
								 }
								case ID_SW:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;	
								case ID_DESKTOP:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_DBUTTON:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_AUTOSCALING:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_DINPUT:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_INPUT:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;	
								case ID_CONN_SAVE_AS:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_CONN_SAVE_ASFAV:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									FillCombo(hwndCombo);
									return 0;
								case ID_CONN_MAN_FAV:
									{
									char szFileName[MAX_PATH];
									if (GetModuleFileName(NULL, szFileName, MAX_PATH))
										{
										char* p = strrchr(szFileName, '\\');
											if (p == NULL) return 0;
											*p = '\0';
										strcat (szFileName,"\\viewers\\");
									}
									ShellExecute(NULL,"open",szFileName,NULL,NULL,SW_SHOW);
									}
									return 0;
								case IDC_OPTIONBUTTON: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_CONN_ABOUT:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VIEWONLYTOGGLE: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;	
								case ID_REQUEST_REFRESH:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VK_LWINDOWN:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VK_LWINUP:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VK_RWINDOWN:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VK_RWINUP:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VK_APPSDOWN:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_VK_APPSUP:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_CONN_CTLESC:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_CONN_CTLALTDEL:
								case ID_BUTTON_CAD :
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;	
								case ID_CONN_CTLDOWN:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;	
								case ID_CONN_CTLUP:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;		
								case ID_CONN_ALTDOWN:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;		
								case ID_CONN_ALTUP:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_FILETRANSFER: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_TEXTCHAT:
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_MAXCOLORS: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_256COLORS: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_HALFSCREEN: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_FUZZYSCREEN: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);		
									return 0;
								case ID_NORMALSCREEN: 
									SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
									return 0;
								case ID_BUTTON_INFO:
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
										return 0;
								case 9998:
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
										return TRUE;
								case ID_BUTTON_SEP:
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),(LPARAM)0);
										return 0;
								case ID_BUTTON_END :
										SendMessage(hwnd_active_child,WM_CLOSE,(WPARAM)0,(LPARAM)0);
										return 0;		
								case ID_BUTTON_PROPERTIES:
											SendMessage(hwnd_active_child,WM_SYSCOMMAND,IDC_OPTIONBUTTON,(LPARAM)0);
											return 0;
								case ID_BUTTON_STRG_ESC:
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_CONN_CTLESC,(LPARAM)0);
										return 0;
								case ID_BUTTON_DINPUT:
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_BUTTON_DINPUT,(LPARAM)0);
										return 0;
								case ID_FULLSCREEN: 
									{
										//SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_FULLSCREEN,(LPARAM)0);
										HMONITOR hMonitor=MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST);
										MONITORINFO mi;
										RECT        rc;
										mi.cbSize = sizeof(mi);
										GetMonitorInfo(hMonitor, &mi);
										int CenterX=(mi.rcMonitor.right-mi.rcMonitor.left);
										TitleBar.Resize(CenterX);
										TitleBar.DisplayWindow(TRUE, TRUE); //Added by: Lars Werner (http://lars.werner.no)
 										TitleBar.SetText("FULL"); //Added by: Lars Werner (http://lars.werner.no)

										ShowWindow(m_hwndMain, SW_MAXIMIZE);

										DWORD style = GetWindowLong(m_hwndMain, GWL_STYLE);
										style &= ~(WS_DLGFRAME | WS_THICKFRAME |WS_SYSMENU |WS_BORDER );
										SetWindowLong(m_hwndMain, GWL_STYLE, style);
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_FULLSCREEN,(LPARAM)0);
										SetMenu(m_hwndMain,NULL);
										HWND hTaskbarWnd = FindWindow( "Shell_TrayWnd", NULL ); 
										ShowWindow( hTaskbarWnd, SW_HIDE); 


//
										SetWindowPos(m_hwndMain, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top-1,
													mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top, SWP_FRAMECHANGED);
										

										fullscreen=true;
										f_StatusBar=false;
										f_ToolBar=false;
										ResizeAllWindows();
										SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_FULLSCREEN,(LPARAM)0);
										return 0;
									}
								case IDD_EXIT:
									PostMessage(hwnd, WM_CLOSE, 0, 0);
									break;
								case ID_NEWCONN:
									if (hwnd_active_child==NULL) app->NewConnection();
									else SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_NEWCONN,(LPARAM)0);
									return 0;
								case ID_NEWCONNF:
									if (hwnd_active_child==NULL) app->NewConnection(g_FileName);
									else SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_NEWCONNF,(LPARAM)0);
									return 0;
								case IDD_STARTLISTEN:
									app->StartListener();
									return 0;
								case IDD_APP_ABOUT:
									ShowAboutBox();	                    
									return 0;
								case IDC_GLOBALOPTIONBUTTON:
									if (hwnd_active_child==NULL) app->NewConnection(true,true,true);
									else SendMessage(hwnd_active_child,WM_SYSCOMMAND,IDC_GLOBALOPTIONBUTTON,(LPARAM)0);
									return 0;

								case ID_SETTINGS_BUTTONSON:
									if (f_ToolBar) f_ToolBar=false;
									else f_ToolBar=true;
									if (f_ToolBar) CheckMenuItem(menu,ID_SETTINGS_BUTTONSON,MF_CHECKED);
									else CheckMenuItem(menu,ID_SETTINGS_BUTTONSON,MF_UNCHECKED);
									{
									RECT rcClient;
									GetClientRect(hwnd, &rcClient);
									if (m_Status==NULL) break;
									if (m_TrafficMonitor)
										{
											SetWindowPos(m_TrafficMonitor, NULL, 380,2, 35,30, SWP_NOSIZE);
											HDC hdcX = GetDC(m_TrafficMonitor);
											HDC hdcBits = CreateCompatibleDC(hdcX);
											SelectObject(hdcBits,m_bitmapNONE);
											RECT rect;
											GetWindowRect(m_TrafficMonitor,&rect);
											BitBlt(hdcX,0,0,rect.right-rect.left,rect.bottom-rect.top,hdcBits,0,0,SRCCOPY);
											DeleteDC(hdcBits);
											ReleaseDC(m_TrafficMonitor,hdcX);
										}
									ResizeAllWindows();
									MoveRebar();
									InvalidateRect(NULL,NULL,true);
									{
										char optionfile[MAX_PATH];
										char *tempvar=NULL;
										tempvar = getenv( "TEMP" );
										strcpy(ini,"");
										if (tempvar) strcpy(ini,tempvar);
										else strcpy(ini,"");
										strcat(ini,"\\options.ini");
										if (f_ToolBar) 
											WritePrivateProfileString("Gloabl","f_ToolBar", "01", ini);
										else WritePrivateProfileString("Gloabl","f_ToolBar", "00", ini);
									}
									}
									break;
								case ID_SETTINGS_AUTORESIZEON:
									if (g_autoscale) g_autoscale=false;
									else g_autoscale=true;
									if (g_autoscale) CheckMenuItem(menu,ID_SETTINGS_AUTORESIZEON,MF_CHECKED);
									else CheckMenuItem(menu,ID_SETTINGS_AUTORESIZEON,MF_UNCHECKED);
									{
										char optionfile[MAX_PATH];
										char *tempvar=NULL;
										tempvar = getenv( "TEMP" );
										strcpy(ini,"");
										if (tempvar) strcpy(ini,tempvar);
										else strcpy(ini,"");
										strcat(ini,"\\options.ini");
										if (g_autoscale) 
											WritePrivateProfileString("Gloabl","g_autoscale", "01", ini);
										else WritePrivateProfileString("Gloabl","g_autoscale", "00", ini);
									}
		

									break;
								case ID_SETTINGS_SAVESESSIONONEXITON:
									if (g_saved_open) g_saved_open=false;
										else g_saved_open=true;
									if (g_saved_open) CheckMenuItem(menu,ID_SETTINGS_SAVESESSIONONEXITON,MF_CHECKED);
									else CheckMenuItem(menu,ID_SETTINGS_SAVESESSIONONEXITON,MF_UNCHECKED);
									{
										char optionfile[MAX_PATH];
										char *tempvar=NULL;
										tempvar = getenv( "TEMP" );
										strcpy(ini,"");
										if (tempvar) strcpy(ini,tempvar);
										else strcpy(ini,"");
										strcat(ini,"\\options.ini");
										if (g_saved_open) 
											WritePrivateProfileString("Gloabl","g_saved_open", "01", ini);
										else WritePrivateProfileString("Gloabl","g_saved_open", "00", ini);
									}
									break;
								case ID_SETTINGS_HOTKEYS:
									{
									char optionfile[MAX_PATH];
									char *tempvar=NULL;
									tempvar = getenv( "TEMP" );
									strcpy(ini,"");
									if (tempvar) strcpy(ini,tempvar);
									else strcpy(ini,"");
									strcat(ini,"\\options.ini");
									HotkeyDlg* dlg = new HotkeyDlg(m_hwndMain, m_hInstResDLL, ini);
									dlg->Go();
									}
									break;
								case ID_SETTINGS_TABBAR:
									if (f_StatusBar) f_StatusBar=false;
									else f_StatusBar=true;
									if (f_StatusBar) CheckMenuItem(menu,ID_SETTINGS_TABBAR,MF_CHECKED);
									else CheckMenuItem(menu,ID_SETTINGS_TABBAR,MF_UNCHECKED);
									{
									RECT rcClient;
									GetClientRect(hwnd, &rcClient);
									if (m_Status==NULL) break;
									if (m_TrafficMonitor)
										{
											SetWindowPos(m_TrafficMonitor, NULL, 380,2, 35,30, SWP_NOSIZE);
											HDC hdcX = GetDC(m_TrafficMonitor);
											HDC hdcBits = CreateCompatibleDC(hdcX);
											SelectObject(hdcBits,m_bitmapNONE);
											RECT rect;
											GetWindowRect(m_TrafficMonitor,&rect);
											BitBlt(hdcX,0,0,rect.right-rect.left,rect.bottom-rect.top,hdcBits,0,0,SRCCOPY);
											DeleteDC(hdcBits);
											ReleaseDC(m_TrafficMonitor,hdcX);
										}
									ResizeAllWindows();
									MoveRebar();
									}
									{
										char optionfile[MAX_PATH];
										char *tempvar=NULL;
										tempvar = getenv( "TEMP" );
										strcpy(ini,"");
										if (tempvar) strcpy(ini,tempvar);
										else strcpy(ini,"");
										strcat(ini,"\\options.ini");
										if (f_StatusBar) 
											WritePrivateProfileString("Gloabl","f_StatusBar", "01", ini);
										else WritePrivateProfileString("Gloabl","f_StatusBar", "00", ini);
									}
									break;




								case CM_WINDOW_TILEHORZ:
									PostMessage(m_hMDIClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
								break;

								case CM_WINDOW_TILEVERT:
									PostMessage(m_hMDIClient, WM_MDITILE, MDITILE_VERTICAL, 0);
								break;

								case CM_WINDOW_CASCADE:
									PostMessage(m_hMDIClient, WM_MDICASCADE, 0, 0);
								break;


								default:
								{
									if(LOWORD(wParam) >= ID_MDI_FIRSTCHILD)
									{
										//DefWindowProc(hwnd, iMsg, wParam, lParam);
										DefFrameProc(hwnd, m_hMDIClient, iMsg, wParam, lParam);
									}
									else
									{
										HWND hChild;
										hChild = (HWND)SendMessage(m_hMDIClient, WM_MDIGETACTIVE, 0, 0);
										if(hChild)
										{
											SendMessage(hChild, WM_COMMAND, wParam, lParam);
										}
									}
									break;
								}
						}//switch
					}//command
					return 0;

				   case WM_CLOSE:
						{
							int iLoop;
							if(g_iNumChild)                                      // If there are any children
							{
								int all=g_iNumChild;
								for(iLoop = 1; iLoop <all; iLoop++)     // Send all children the WM_CLOSE to free up their DC and RC
								{
//Does not work 
									if (g_saved_open) SendMessage(g_child[iLoop].hWnd,WM_SYSCOMMAND,(WPARAM)ID_CONN_SAVE_ASFAV_ALL,(LPARAM)0);
									
								}
								for(iLoop = 1; iLoop <all; iLoop++)     // Send all children the WM_CLOSE to free up their DC and RC
								{
								SendMessage(g_child[1].hWnd,WM_CLOSE,0, 0);
								}
							}
							delete[] g_child;                                     // Delete the array of children
							DestroyWindow(hwnd);
						}
						break;

					case WM_SIZE:
						{
						RECT rcClient;
						GetClientRect(hwnd, &rcClient);
						if (m_Status==NULL) break;
						if (m_TrafficMonitor)
							{
								SetWindowPos(m_TrafficMonitor, NULL, 380,2, 35,30, SWP_NOSIZE);
								HDC hdcX = GetDC(m_TrafficMonitor);
								HDC hdcBits = CreateCompatibleDC(hdcX);
								SelectObject(hdcBits,m_bitmapNONE);
								RECT rect;
								GetWindowRect(m_TrafficMonitor,&rect);
								BitBlt(hdcX,0,0,rect.right-rect.left,rect.bottom-rect.top,hdcBits,0,0,SRCCOPY);
								DeleteDC(hdcBits);
								ReleaseDC(m_TrafficMonitor,hdcX);
							}
						ResizeAllWindows();
						MoveRebar();
						}
						//break;
						return 0; 
 
					case WM_NOTIFY: 
						{
							NMHDR* pNMHDR = (NMHDR*) lParam;
							switch (pNMHDR->code) { 

								case TCN_SELCHANGING:
									{
										int iPage = TabCtrl_GetCurSel(m_hwndTab); 
										vnclog.Print(0, _T("Sel %i\n"),iPage);
										if (IsZoomed(hwnd_active_child))
										{
											Zoomed=true;
										}
										else Zoomed=false;
									}
									break;
 
								case TCN_SELCHANGE: { 
//														Beep(100,100);
														int iPage = TabCtrl_GetCurSel(m_hwndTab); 
														if (iPage !=0)
														{
														SendMessage(m_hMDIClient, WM_MDIACTIVATE, (WPARAM) g_child[iPage].hWnd, 0); 
														if (Zoomed)
															SendMessage(m_hMDIClient, WM_MDIMAXIMIZE, (WPARAM) g_child[iPage].hWnd, 0);
														}
													}
													break;
								case NM_CLICK:
													{
														{
															int iPage = TabCtrl_GetCurSel(m_hwndTab); 
															if (iPage ==0 && pNMHDR->idFrom==0)
															{
																if (hwnd_active_child==NULL) app->NewConnection();
																else SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_NEWCONN,(LPARAM)0);
															}
														}
													} 
													break; 
							} 
						}
							break;


						case WM_SETFOCUS:
							if (m_hMDIClient!=NULL) SendMessage(m_hMDIClient,WM_SETFOCUS,NULL,NULL);
		//					Beep(500,100);
							break;

						case WM_HOTKEY:
							{
								switch (wParam)
								{
									case HOTKEY_CAD:
										SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_CONN_CTLALTDEL),(LPARAM)0);
										break;
									case HOTKEY_FULL:
										SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_FULLSCREEN),(LPARAM)0);
										break;
									case HOTKEY_REFRESH:
										SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_REQUEST_REFRESH),(LPARAM)0);
										break;
									case HOTKEY_SHOWCON:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(IDC_OPTIONBUTTON),(LPARAM)0);
										break;
									case HOTKEY_CE:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_CONN_CTLESC),(LPARAM)0);
										break;
									case HOTKEY_CUS:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_BUTTON_SEP),(LPARAM)0);
										break;
									case HOTKEY_STATUS:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_BUTTON_INFO),(LPARAM)0);
										break;
									case HOTKEY_CLOSE:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_BUTTON_END),(LPARAM)0);
										break;
									case HOTKEY_HIDE:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_DBUTTON),(LPARAM)0);
										break;
									case HOTKEY_BLANK:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_BUTTON_DINPUT),(LPARAM)0);
										break;
									case HOTKEY_FILE:
										SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_FILETRANSFER),(LPARAM)0);
										break;
									case HOTKEY_SINGLE:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_SW),(LPARAM)0);
										break;
									case HOTKEY_FULLDESK:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_DESKTOP),(LPARAM)0);
										break;
									case HOTKEY_CHAT:
											SendMessage(hwnd,WM_COMMAND,(WPARAM)LOWORD(ID_TEXTCHAT),(LPARAM)0);
										break;
								}
							}
							break;
								


						case WM_DESTROY:
							UnregisterHotKey(m_hwndMain, 0);
							UnregisterHotKey(m_hwndMain, 1);
							UnregisterHotKey(m_hwndMain, 2);
							UnregisterHotKey(m_hwndMain, 3);
							UnregisterHotKey(m_hwndMain, 4);
							UnregisterHotKey(m_hwndMain, 5);
							UnregisterHotKey(m_hwndMain, 6);
							UnregisterHotKey(m_hwndMain, 7);
							UnregisterHotKey(m_hwndMain, 8);
							UnregisterHotKey(m_hwndMain, 9);
							UnregisterHotKey(m_hwndMain, 10);
							UnregisterHotKey(m_hwndMain, 11);
							UnregisterHotKey(m_hwndMain, 12);
							UnregisterHotKey(m_hwndMain, 13);
							{HWND hTaskbarWnd = FindWindow( "Shell_TrayWnd", NULL ); 
							ShowWindow( hTaskbarWnd, SW_SHOW ); 
							g_stop=true;
							PostQuitMessage(0);}
							break;
					
						case tbWM_CLOSE:
							{HWND hTaskbarWnd = FindWindow( "Shell_TrayWnd", NULL ); 
							ShowWindow( hTaskbarWnd, SW_SHOW ); 
							SendMessage(m_hwndMain, WM_CLOSE,NULL,NULL);}
							return 0;

						case tbWM_MINIMIZE:
							{HWND hTaskbarWnd = FindWindow( "Shell_TrayWnd", NULL ); 
							ShowWindow( hTaskbarWnd, SW_SHOW ); 
							ShowWindow(m_hwndMain, SW_MINIMIZE);}
							//Beep(100,100);
							return 0;

						case tbWM_MAXIMIZE:
							{
							HWND hTaskbarWnd = FindWindow( "Shell_TrayWnd", NULL ); 
							ShowWindow( hTaskbarWnd, SW_SHOW ); 
							TitleBar.DisplayWindow(FALSE,TRUE); //Added by: Lars Werner (http://lars.werner.no)
 							TitleBar.SetText("FULL"); //Added by: Lars Werner (http://lars.werner.no)
							SetMenu(m_hwndMain,menu);
							ShowWindow(m_hwndMain, SW_RESTORE);

							DWORD style = GetWindowLong(m_hwndMain, GWL_STYLE);
							style |= (WS_DLGFRAME | WS_THICKFRAME |WS_SYSMENU |WS_BORDER );
							SetWindowLong(m_hwndMain, GWL_STYLE, style);
							SetWindowPos(m_hwndMain, HWND_NOTOPMOST, 0,0,100,100, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
							f_StatusBar=true;
							f_ToolBar=true;
							fullscreen=false;
							ResizeAllWindows();
							MoveRebar();
							SendMessage(hwnd_active_child,WM_SYSCOMMAND,ID_OUTFULLSCREEN,(LPARAM)0);
							PostMessage(m_hMDIClient, WM_MDICASCADE, 0, 0);
							return 0;
							}
			} // end of iMsg switch
	return DefFrameProc(hwnd, m_hMDIClient, iMsg, wParam, lParam);;
#pragma warning(disable : 4101)
}






void StartWindows(VNCviewerApp *appl,bool fromconfig)
{
	InitCommonControls(); 
	app=appl;
	CreateDisplay();
	ToolBar(m_hwndMain);
	CreateStatusWindow();
	FillCombo(hwndCombo);
	CLIENTCREATESTRUCT ccs;
    ccs.hWindowMenu = NULL;
    ccs.idFirstChild = ID_MDI_FIRSTCHILD;

    m_hMDIClient = CreateWindowEx(NULL, "MDICLIENT", NULL,
    WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE|WS_BORDER,                    
    0,0,0,0,
    m_hwndMain, NULL, m_hInstResDLL, (LPVOID)&ccs);
	ResizeAllWindows();
	MoveRebar();
	EnableMenuItem(menu, 1, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(menu, 2, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(menu, 3, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(menu, 4, MF_BYPOSITION | MF_GRAYED);
	if (!fromconfig){
		HANDLE fileh;
		WIN32_FIND_DATA finddata;
		char WORKDIR[MAX_PATH];
		char FULLFILL[MAX_PATH];
		char FILENAME[MAX_PATH];
		bool is;
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
					{
					char* p = strrchr(WORKDIR, '\\');
						if (p == NULL) return;
						*p = '\0';
					}
		strcpy(FULLFILL,"");
		strcat(FULLFILL,WORKDIR);//set the directory
		strcat(FULLFILL,"\\temp\\*.vnc");//set the filter

		fileh=FindFirstFile(FULLFILL,&finddata);
		strcat(WORKDIR,"\\temp\\");
		if (fileh == INVALID_HANDLE_VALUE) 
		{
		}
		else
		{
		strcpy(FULLFILL,"");
		strcat(FULLFILL,WORKDIR);
		strcat(FULLFILL,finddata.cFileName);
		strcpy(g_FileName,FULLFILL);

		SendMessage(m_hwndMain,WM_COMMAND,ID_NEWCONNF,0);
		//app->NewConnection(FULLFILL);

		DeleteFile(FULLFILL);
		is=FindNextFile(fileh,&finddata);
		while(is)
			{
			if(finddata.dwFileAttributes!=FILE_ATTRIBUTE_DIRECTORY )//we dont want any directory listings do we? 
				{
					if(is)
					{
						strcpy(FULLFILL,"");
						strcat(FULLFILL,WORKDIR);
						strcat(FULLFILL,finddata.cFileName);
						strcpy(g_FileName,FULLFILL);

						SendMessage(m_hwndMain,WM_COMMAND,ID_NEWCONNF,0);
						//app->NewConnection(FULLFILL);

						DeleteFile(FULLFILL);
					}
				}
			is=FindNextFile(fileh,&finddata);
			}
		}
	}
	else 
	{
		g_saved_open=false;
		SendMessage(m_hwndMain,WM_COMMAND,ID_NEWCONN,0);
	}
}





///////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ResizeAllWindows( void )
{
	RECT rc,rcClient;
	short cxClientMain, cyClientMain;
	//int cyClientMain;
	//int cxClientMain;

	GetClientRect( m_hwndMain, &rc );
	cxClientMain = rc.right - rc.left;
	cyClientMain = rc.bottom - rc.top;

	if( f_ToolBar == FALSE )
	{
		if( IsWindowVisible( hWndRebar) )
		{	
			ShowWindow( hWndRebar, SW_HIDE );
			g_cyToolBar=0;
		}
	}
	else
	{
		if( !IsWindowVisible( hWndRebar ))
		{
			ShowWindow( hWndRebar, SW_SHOWDEFAULT );
			g_cyToolBar = 32;
		}
	}


	if( f_StatusBar == FALSE )
	{
		if( IsWindowVisible( m_Status) )
		{
			ShowWindow( m_Status, SW_HIDE );
			g_cyStatusBar = 0;
		}
	}
	else
	{
		if( !IsWindowVisible( m_Status ) )
		{
			ShowWindow( m_Status, SW_SHOWDEFAULT );
			g_cyStatusBar = g_cyStatusBar0;
		}
	}
	MoveWindow( m_Status, 0, g_cyToolBar,
					cxClientMain, g_cyStatusBar, TRUE );

	MoveWindow( m_hMDIClient, 0, g_cyToolBar+g_cyStatusBar, cxClientMain,cyClientMain - g_cyToolBar - g_cyStatusBar, TRUE );

	GetClientRect( m_hwndMain, &rc );
    GetClientRect(m_Status, &rcClient); 
	MoveWindow( m_hwndTab, 0, 0, rc.right-rc.left, rcClient.bottom, TRUE );  


	return TRUE;
}



static void MoveRebar( void )
{
	RECT rc;
	int x, y, cx, cy;
	x = y = 0; // for future.
	GetWindowRect( m_hwndMain, &rc );
	cx = rc.right - rc.left;
	cy = rc.bottom - rc.top;

	MoveWindow( hWndRebar,x, y,cx, 28, TRUE );
}

///////////////////////////////////////////////////////////////////////////////////////////////////
				
				

static void
ToolBar( HWND hWndParent )
{
	bool f_rebar=true;

	int index;
	RECT rc;
	HWND m_hwndTT;
	int row,col;
	TOOLINFO ti;
	int id=0;

	TBADDBITMAP tbab; 
		TBBUTTON tbButtons []=
		{
			{0,ID_CONN_CTLALTDEL,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,0},
			{1,ID_FULLSCREEN,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,1},
			{2,IDC_OPTIONBUTTON,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,2},
			{3,ID_REQUEST_REFRESH,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,3},
			{4,ID_CONN_CTLESC,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,4},
			{5,ID_BUTTON_SEP,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,5},
			{6,ID_BUTTON_INFO,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,6},
			{7,ID_BUTTON_END,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,7},
			{8,ID_SETTINGS_BUTTONSON,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,8},
			{9,ID_BUTTON_DINPUT,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,9},
			{10,ID_FILETRANSFER,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,10},
			{11,ID_SW,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,11},
			{12,ID_DESKTOP,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,12},
			{13,ID_TEXTCHAT,TBSTATE_ENABLED,TBSTYLE_BUTTON,0L,13},
			
		};

	static char *szTips[14] = 
		{
				sz_L2,
				sz_L3,
				sz_L4,
				sz_L5,
				sz_L6,
				sz_L7,
				sz_L8,
				sz_L9,
				sz_L10,
				sz_L11,
				sz_L12,
				sz_L13,
				sz_L14,
				sz_L15,
		};
	f_rebar = TRUE;
	if( f_rebar )
	{
		REBARINFO rbi;
		GetWindowRect( hWndParent, &rc );
		hWndRebar = CreateWindowEx( WS_EX_TOOLWINDOW,
			REBARCLASSNAME,
			NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
			WS_CLIPCHILDREN | RBS_VARHEIGHT | CCS_NODIVIDER,
			0, 0, 0, 0,
			hWndParent, (HMENU)IDR_MENU1, m_hInstResDLL, NULL );
		ZeroMemory( &rbi, sizeof(REBARINFO) );
		rbi.cbSize = sizeof(REBARINFO);
		rbi.fMask = 0;
		rbi.himl = (HIMAGELIST)NULL;
		SendMessage( hWndRebar, RB_SETBARINFO, 0, (LPARAM)&rbi );
	}

	
	hWndToolBar = CreateToolbarEx(
				hWndParent,
				WS_VISIBLE | WS_CHILD | TBSTYLE_TOOLTIPS  |CCS_NOMOVEY|TBSTYLE_FLAT|WS_CLIPSIBLINGS
//WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS |TBSTYLE_FLAT|WS_CLIPSIBLINGS
				,IDR_TOOLBAR
				,14
				,m_hInstResDLL
				,IDB_BITMAP8
				,(LPCTBBUTTON)&tbButtons
				,14
				,20
				,20
				,20
				,20
				,sizeof(TBBUTTON));

	RECT trect;
	SendMessage(hWndToolBar,TB_SETROWS,(WPARAM) MAKEWPARAM (1, true),(LPARAM) (LPRECT) (&trect));
		
		
	m_hwndTT = CreateWindow(
			TOOLTIPS_CLASS,
			(LPSTR)NULL,
			TTS_ALWAYSTIP,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			(HMENU)NULL,
			m_hInstResDLL,
			NULL);
		
		DWORD buttonWidth = LOWORD(SendMessage(hWndToolBar,TB_GETBUTTONSIZE,(WPARAM)0,(LPARAM)0));
		DWORD buttonHeight = HIWORD(SendMessage(hWndToolBar,TB_GETBUTTONSIZE,(WPARAM)0,(LPARAM)0));
		
		for (row = 0; row < 1 ; row++ ) 
			for (col = 0; col < 14; col++) { 
				ti.cbSize = sizeof(TOOLINFO); 
				ti.uFlags = 0 ; 
				ti.hwnd = hWndToolBar; 
				ti.hinst = m_hInstResDLL; 
				ti.uId = (UINT) id; 
				ti.lpszText = (LPSTR) szTips[id++]; 
				ti.rect.left = col * buttonWidth; 
				ti.rect.top = row * buttonHeight; 
				ti.rect.right = ti.rect.left + buttonWidth; 
				ti.rect.bottom = ti.rect.top + buttonHeight; 
				
				SendMessage(m_hwndTT, TTM_ADDTOOL, 0, 
                    (LPARAM) (LPTOOLINFO) &ti); 
				
			}
	SendMessage(hWndToolBar,TB_SETTOOLTIPS,(WPARAM)(HWND)m_hwndTT,(LPARAM)0);
	SendMessage(m_hwndTT,TTM_SETTIPBKCOLOR,(WPARAM)(COLORREF)0x00404040,(LPARAM)0);
	SendMessage(m_hwndTT,TTM_SETTIPTEXTCOLOR,(WPARAM)(COLORREF)0x00F5B28D,(LPARAM)0);
	SendMessage(m_hwndTT,TTM_SETDELAYTIME,(WPARAM)(DWORD)TTDT_INITIAL,(LPARAM)(INT) MAKELONG(200,0));
		
	RECT clr;
	GetClientRect(hWndToolBar,&clr);
	m_TrafficMonitor = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
											"Static",
											"",
											WS_CHILD | WS_VISIBLE |SS_BITMAP ,
											380,
											2,
											35,
											30,
											hWndToolBar,
											NULL,
											m_hInstResDLL,
											NULL);
	ShowWindow(m_TrafficMonitor,SW_SHOW);

	m_bitmapNONE = LoadImage(m_hInstResDLL,MAKEINTRESOURCE(IDB_STAT_NONE),IMAGE_BITMAP,22,20,LR_SHARED);
	m_bitmapFRONT = LoadImage(m_hInstResDLL,MAKEINTRESOURCE(IDB_STAT_FRONT),IMAGE_BITMAP,22,20,LR_SHARED);
	m_bitmapBACK= LoadImage(m_hInstResDLL,MAKEINTRESOURCE(IDB_STAT_BACK),IMAGE_BITMAP,22,20,LR_SHARED);
	SendMessage(m_TrafficMonitor, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_bitmapNONE);


	if( f_rebar )
		{
			if( hWndRebar )
			{

				REBARBANDINFO rbbi;
				RECT rc;
				DWORD dwHeight;
				ZeroMemory( &rbbi, sizeof(REBARBANDINFO) );
				// Common parameters 
				rbbi.cbSize		  = sizeof(REBARBANDINFO);
				rbbi.fMask		  = //RBBIM_COLORS | 
										RBBIM_SIZE | 
										RBBIM_CHILD | 
										RBBIM_CHILDSIZE | 
										RBBIM_STYLE | 
										RBBIM_TEXT;
				rbbi.fStyle		  = RBBS_CHILDEDGE ;//| RBBS_GRIPPERALWAYS;


				dwHeight = SendMessage( hWndToolBar, TB_GETBUTTONSIZE, 0, 0 );
				rbbi.lpText		= "";
				rbbi.hwndChild	= hWndToolBar;
				rbbi.cxMinChild = 350;
				rbbi.cyMinChild = 28;
				rbbi.cx			  = 350;
				SendMessage( hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi );
				MoveRebar();

			}
		}

	RECT rc_tmp;
	GetWindowRect( hWndToolBar, &rc_tmp );
	g_cyToolBar=g_cyToolBar0 = rc_tmp.bottom - rc_tmp.top;

	{
	REBARBANDINFO rbbi;
	HWND hwndChild = CreateWindowEx(   0, 
                                 TEXT("button"), 
                                 TEXT("Connect"),
                                 WS_CHILD | 
                                    BS_PUSHBUTTON | 
                                    0,
                                 0, 
                                 4, 
                                 100, 
                                 10, 
                                 hWndToolBar, 
                                 (HMENU)ID_BUTTONSELECT, 
                                 m_hInstResDLL, 
                                 NULL);
   
   memset (&rbbi, 0, sizeof (rbbi));
   rbbi.cbSize       = sizeof(REBARBANDINFO);
   rbbi.fMask        = RBBIM_SIZE | 
                        RBBIM_CHILD | 
                        RBBIM_CHILDSIZE | 
                        RBBIM_ID | 
                        RBBIM_STYLE | 
                        RBBIM_TEXT |
                        RBBIM_BACKGROUND |
                        0;
   rbbi.cxMinChild   = 80;
   rbbi.cyMinChild   = 24;
   rbbi.cx           = 80;
   rbbi.fStyle       = RBBS_CHILDEDGE | 
                        RBBS_FIXEDBMP |
                        RBBS_GRIPPERALWAYS |
                        0;
   rbbi.wID          = ID_BUTTONSELECT;
   rbbi.hwndChild    = hwndChild;
   rbbi.lpText       = TEXT("Connect");
   rbbi.hbmBack      = NULL;//LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BACKGROUND));


   SendMessage(hWndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)(LPREBARBANDINFO)&rbbi);
   }
	{
		#define ID_COMBOBOX 55555
		RECT rect;
		REBARBANDINFO rbbi;
		TCHAR szString[64];         // A temporary string
		DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | 
            WS_CLIPCHILDREN | WS_CLIPSIBLINGS | 
            CBS_AUTOHSCROLL | CBS_DROPDOWN|CCS_NOPARENTALIGN |CCS_NOMOVEY;


		hwndCombo = CreateWindowEx (0, 
                                    TEXT("combobox"), 
                                    NULL, 
                                    dwStyle, 
                                    0, 4, 100, 200, 
                                    hWndToolBar, 
                                    (HMENU)NULL, 
                                    m_hInstResDLL, 
                                    NULL);

	  // Select the first item as the default.
	  SendMessage (hwndCombo, CB_SETCURSEL, (WPARAM)0, 0);

	  // Retrieve the dimensions of the bounding rectangle of the combo box. 
	  GetWindowRect (hwndCombo, &rect);

  memset (&rbbi, 0, sizeof (rbbi));
  rbbi.cbSize = sizeof (REBARBANDINFO);
  rbbi.fMask = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID 
                  | RBBIM_STYLE | RBBIM_TEXT | RBBIM_BACKGROUND 
                  | RBBIM_IMAGE | 0;

  rbbi.cxMinChild = 0;
  rbbi.cyMinChild = 24;
  rbbi.cx = 500;
  rbbi.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP | RBS_BANDBORDERS;
  rbbi.wID = ID_COMBOBOX;
  rbbi.hwndChild = hwndCombo;
  rbbi.lpText = "";
  rbbi.hbmBack = NULL;//LoadBitmap (m_hInstResDLL, MAKEINTRESOURCE (IDB_BKGRD));
  rbbi.iImage = 0;

  // Insert the combo box band in the rebar control. 
  SendMessage (hWndRebar, RB_INSERTBAND, (WPARAM)-1, 
               (LPARAM) (LPREBARBANDINFO)&rbbi);
	}
	return;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
static void CreateStatusWindow()
{
	RECT rcClient,rc; 
    TCITEM tie; 
    int i; 

	m_Status = CreateWindowEx(0L, STATUSCLASSNAME, "",
											WS_CHILD|CCS_TOP|CCS_NOPARENTALIGN  ,
											0,0,0,0,
											m_hwndMain,
											NULL,
											m_hInstResDLL,
											NULL);
	GetClientRect( m_hwndMain, &rc );
	ShowWindow(m_Status,SW_SHOW); 
    GetClientRect(m_Status, &rcClient); 
    m_hwndTab = CreateWindowEx(0, 
        WC_TABCONTROL, "", 
        WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS |TCS_BOTTOM |TCS_TABS|TCS_FLATBUTTONS , 
        0, 0, rc.right-rc.left, rcClient.bottom, 
        m_Status, NULL, m_hInstResDLL, NULL 
        ); 
    if (m_hwndTab == NULL) 
        return; 
	tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 
    tie.pszText = "New "; 
	TabCtrl_InsertItem(m_hwndTab,0,&tie);
	TabCtrl_SetMinTabWidth(m_hwndTab,9);
//	TabCtrl_SetItemSize(m_hwndTab,9,rcClient.bottom/2);
	g_iNumChild++;
	RECT rc_tmp;
	GetWindowRect( m_Status, &rc_tmp );
	g_cyStatusBar = g_cyStatusBar0 = rc_tmp.bottom - rc_tmp.top;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
static
void CreateTrafficWindow()
{
	RECT clr;
	RECT r;
	GetClientRect(m_hwndMain,&clr);
	m_TrafficMonitor = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
											"Static",
											"",
											WS_CHILD | WS_VISIBLE |SS_BITMAP ,
											380,
											2,
											35,
											30,
											hWndToolBar,
											NULL,
											m_hInstResDLL,
											NULL);
	ShowWindow(m_TrafficMonitor,SW_SHOW);

	m_bitmapNONE = LoadImage(m_hInstResDLL,MAKEINTRESOURCE(IDB_STAT_NONE),IMAGE_BITMAP,22,20,LR_SHARED);
	m_bitmapFRONT = LoadImage(m_hInstResDLL,MAKEINTRESOURCE(IDB_STAT_FRONT),IMAGE_BITMAP,22,20,LR_SHARED);
	m_bitmapBACK= LoadImage(m_hInstResDLL,MAKEINTRESOURCE(IDB_STAT_BACK),IMAGE_BITMAP,22,20,LR_SHARED);
	SendMessage(m_TrafficMonitor, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_bitmapNONE);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CreateDisplay()
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hInstResDLL;
    wc.hIcon = LoadIcon(m_hInstResDLL, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DSHADOW+1);
	menu=LoadMenu(m_hInstResDLL,MAKEINTRESOURCE(IDR_MAIN));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("VNCMDI_Window");;
    wc.hIconSm = LoadIcon(m_hInstResDLL, MAKEINTRESOURCE(IDI_ICON1));
	RegisterClassEx(&wc);

	const DWORD winstyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	m_hwndMain = CreateWindowEx(WS_EX_LEFT,_T("VNCMDI_Window"),
			  _T("VNCviewer"),
			  winstyle,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  NULL,                // Parent handle
			  menu,                // Menu handle
			  m_hInstResDLL,
			  NULL);
	ShowWindow(m_hwndMain,SW_SHOW);
	if(TitleBar.GetSafeHwnd()==NULL) 
		TitleBar.Create(m_hInstResDLL, m_hwndMain);
	TitleBar.DisplayWindow(FALSE, TRUE);
	{
		char optionfile[MAX_PATH];
		char *tempvar=NULL;
		tempvar = getenv( "TEMP" );
		strcpy(ini,"");
		if (tempvar) strcpy(ini,tempvar);
		else strcpy(ini,"");
		strcat(ini,"\\options.ini");
		char i[3];
		GetPrivateProfileString("Gloabl","f_StatusBar", "00", i, 3, ini);
		if (strcmp("00",i)==NULL) f_StatusBar=false;
		else f_StatusBar=true;
		GetPrivateProfileString("Gloabl","f_ToolBar", "00", i, 3, ini);
		if (strcmp("00",i)==NULL)f_ToolBar=false;
		else f_ToolBar=true;
		GetPrivateProfileString("Gloabl","g_saved_open", "00", i, 3, ini);
			if (strcmp("00",i)==NULL)g_saved_open=false;
		else g_saved_open=true;
		GetPrivateProfileString("Gloabl","g_autoscale", "00", i, 3, ini);
		if (strcmp("00",i)==NULL) g_autoscale=false;
		else g_autoscale=true;
		if (f_ToolBar) CheckMenuItem(menu,ID_SETTINGS_BUTTONSON,MF_CHECKED);
		else CheckMenuItem(menu,ID_SETTINGS_BUTTONSON,MF_UNCHECKED);
		if (f_StatusBar) CheckMenuItem(menu,ID_SETTINGS_TABBAR,MF_CHECKED);
		else CheckMenuItem(menu,ID_SETTINGS_TABBAR,MF_UNCHECKED);
		if (g_saved_open) CheckMenuItem(menu,ID_SETTINGS_SAVESESSIONONEXITON,MF_CHECKED);
		else CheckMenuItem(menu,ID_SETTINGS_SAVESESSIONONEXITON,MF_UNCHECKED);
		if (g_autoscale) CheckMenuItem(menu,ID_SETTINGS_AUTORESIZEON,MF_CHECKED);
		else CheckMenuItem(menu,ID_SETTINGS_AUTORESIZEON,MF_UNCHECKED);
		SetupHotkey("Cad", HOTKEY_CAD,ini);
		SetupHotkey("Full", HOTKEY_FULL,ini);
		SetupHotkey("Refresh", HOTKEY_REFRESH,ini);
		SetupHotkey("Showcon", HOTKEY_SHOWCON,ini);
		SetupHotkey("Ce", HOTKEY_CE,ini);
		SetupHotkey("Cus", HOTKEY_CUS,ini);
		SetupHotkey("Status", HOTKEY_STATUS,ini);
		SetupHotkey("Close", HOTKEY_CLOSE,ini);
		SetupHotkey("Hide", HOTKEY_HIDE,ini);
		SetupHotkey("Blank", HOTKEY_BLANK,ini);
		SetupHotkey("File", HOTKEY_FILE,ini);
		SetupHotkey("Single", HOTKEY_SINGLE,ini);
		SetupHotkey("Fulldesk", HOTKEY_FULLDESK,ini);
		SetupHotkey("Chat", HOTKEY_CHAT,ini);
	}
	
}

void SetupHotkey(char* name, int id,char* ini)
{
	char i[4]="";
	GetPrivateProfileString("Hotkeys", name, "000", i, 4, ini);
	int mi = i[0]-'0';
	int ki = atoi(i+1);

	if (mi > 0 && ki > 0)
	{
		UINT mod;
		UINT key;

		if (mi == 1) mod = MOD_WIN;
		else if (mi == 2) mod = MOD_CONTROL;
		else if (mi == 3) mod = MOD_CONTROL | MOD_ALT;
		else if (mi == 4) mod = MOD_CONTROL | MOD_SHIFT;
		else if (mi == 5) mod = MOD_SHIFT | MOD_ALT;

		if (ki <= 26) key = 'A' + ki - 1;	
		else if (ki <= 36) key = '0' + ki - 27;
		else if (ki <= 48) key = VK_F1 + ki - 37;
		else if (ki == 49) key = VK_RIGHT;
		else if (ki == 50) key = VK_LEFT;
		else if (ki == 51) key = VK_UP;
		else if (ki == 52) key = VK_DOWN;
		else if (ki == 53) key = VK_HOME;
		else if (ki == 54) key = VK_END;
		else if (ki == 55) key = VK_INSERT;

		if (!RegisterHotKey(m_hwndMain, id, mod, key))
		{
			char errMsg[256] = "";
			sprintf(errMsg, "Unable to register hotkey for %s.", name);
			MessageBox(m_hwndMain, errMsg, "Hotkey Registration Error", MB_SYSTEMMODAL | MB_OK | MB_ICONERROR);
		}
	}
}
