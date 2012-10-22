#include "stdafx.h"
#include "resource.h"
#include <time.h>
#include "upnp.h"
#include "firewall.h"
#include "log.h"
#include <iphlpapi.h>
#pragma comment ( lib, "iphlpapi" )
#include <shlwapi.h>
#pragma comment ( lib, "shlwapi" )
#ifndef _WINSOCK2API_
	#include <winsock2.h>
	#pragma comment(lib, "ws2_32.lib")
#endif
extern int testport;
extern char strLocalIP[256];
extern char strExternIP[256];
extern HWND hTab7dialog;
extern HWND hTab6dialog;
char servercmd[256];
char viewercmd[256];
char strrepeater[256];
char strid[256];
extern bool manual;
bool set=false;
char Viewerport[10];
char Serverport[10];
void createsfx3(char *cmd,char *cmd2);

BOOL CALLBACK DlgProcSFX2(HWND hwnd, UINT uMsg,WPARAM wParam, LPARAM lParam)
{	
	switch (uMsg) {
		
	case 24:
		set=true;
		if (SendMessage(GetDlgItem(hwnd, IDC_CHECKRECON), BM_GETCHECK, 0, 0)== BST_CHECKED)
		{
			strcpy(servercmd,"-autoreconnect -connect ");
		}
		else
		{
			strcpy(servercmd,"-connect ");
		}
		SetWindowText(GetDlgItem(hwnd, IDC_LOCALIP),strLocalIP);
		SetWindowText(GetDlgItem(hwnd, IDC_REMOTEIP),strExternIP);
		GetWindowText(GetDlgItem(hwnd, IDC_EDITSPORT),Serverport,10);
		GetWindowText(GetDlgItem(hwnd, IDC_EDITVPORT),Viewerport,10);

		if (strlen(strLocalIP)!=0)
		{
			if (SendMessage(GetDlgItem(hwnd, IDC_RADIOINTERNET), BM_GETCHECK, 0, 0)== BST_CHECKED)
			{
				char temp[10];
				bool autore;
				bool rep;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKRECON), BM_GETCHECK, 0, 0)== BST_CHECKED)
					autore=true;
				else autore=false;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKREP), BM_GETCHECK, 0, 0)== BST_CHECKED)
				rep=true;
				else rep=false;

				if (!rep && autore)
					{
						strcpy(servercmd,"-autoreconnect -connect ");
						strcat(servercmd,strExternIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if (!rep && !autore)
					{
						strcpy(servercmd,"-connect ");
						strcat(servercmd,strExternIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if( rep && autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-autoreconnect -ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}
				else if(rep && !autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}


				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
					{
						strcat(servercmd," -dsmplugin MSRC4Plugin.dsm");
					}
				strcat(servercmd," -run");
			}
			else
			{
				char temp[10];
				bool autore;
				bool rep;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKRECON), BM_GETCHECK, 0, 0)== BST_CHECKED)
					autore=true;
				else autore=false;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKREP), BM_GETCHECK, 0, 0)== BST_CHECKED)
				rep=true;
				else rep=false;

				if (!rep && autore)
					{
						strcpy(servercmd,"-autoreconnect -connect ");
						strcat(servercmd,strLocalIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if (!rep && !autore)
					{
						strcpy(servercmd,"-connect ");
						strcat(servercmd,strLocalIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if( rep && autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-autoreconnect -ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}
				else if(rep && !autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
					{
						strcat(servercmd," -dsmplugin MSRC4Plugin.dsm");

					}
				strcat(servercmd," -run");
			}
			SetDlgItemText(hwnd,IDC_EDITOUTSERV,servercmd);

			if (SendMessage(GetDlgItem(hwnd, IDC_CHECKREP), BM_GETCHECK, 0, 0)== BST_CHECKED)
			{
				strcpy(viewercmd,"-autoreconnect 3");
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
					{
						strcat(viewercmd," -dsmplugin MSRC4Plugin.dsm ");
					}
						strcat(viewercmd,"-proxy ");
						strcat(viewercmd,strrepeater);
						strcat(viewercmd,":");
						strcat(viewercmd,Viewerport);
						strcat(viewercmd," ID");
						strcat(viewercmd,":");
						strcat(viewercmd,strid);
			}
			else
			{
				char temp[10];
				strcpy(viewercmd,"-listen ");
				strcat(viewercmd,_itoa(testport,temp,10));
				strcat(viewercmd," -autoreconnect 3");

				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
						{
							strcat(viewercmd," -dsmplugin MSRC4Plugin.dsm");

						}
			}
			SetDlgItemText(hwnd,IDC_EDITOUTVIEW,viewercmd);
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOINTERNET),true);
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOLAN), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSESERVICE), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKRECON), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKREP), true);
			if (SendDlgItemMessage(hwnd, IDC_CHECKREP, BM_GETCHECK, 0, 0))
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITID), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), true);
			}
			else
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITID), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), false);
			}
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTSERV), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTVIEW), true);

			EnableWindow(GetDlgItem(hwnd, IDC_CREATESFX), true);
			EnableWindow(GetDlgItem(hwnd, IDC_LOCALIP), true);
			EnableWindow(GetDlgItem(hwnd, IDC_REMOTEIP), true);
		}
		else if (manual)
		{
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOINTERNET),true);
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOLAN), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSESERVICE), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKRECON), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKREP), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITID), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTSERV), true);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTVIEW), true);

			EnableWindow(GetDlgItem(hwnd, IDC_CREATESFX), true);
			if (SendDlgItemMessage(hwnd, IDC_CHECKREP, BM_GETCHECK, 0, 0))
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITID), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), true);
			}
			else
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITID), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), false);
			}
			EnableWindow(GetDlgItem(hwnd, IDC_LOCALIP), true);
			EnableWindow(GetDlgItem(hwnd, IDC_REMOTEIP), true);
		}
		else
		{
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOINTERNET),false);
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOLAN), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSESERVICE), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKRECON), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKREP), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITID), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTSERV), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTVIEW), false);

			EnableWindow(GetDlgItem(hwnd, IDC_CREATESFX), false);
			EnableWindow(GetDlgItem(hwnd, IDC_LOCALIP), false);
			EnableWindow(GetDlgItem(hwnd, IDC_REMOTEIP), false);
		}
		set=false;
		break;
	case WM_INITDIALOG: 
		{	
			srand ( time(NULL) );
			int nummer=rand() % 8999 + 1000;
			if (SendMessage(GetDlgItem(hwnd, IDC_CHECKRECON), BM_GETCHECK, 0, 0)== BST_CHECKED)
				{
					strcpy(servercmd,"-autoreconnect -connect ");
				}
				else
				{
					strcpy(servercmd,"-connect ");
				}
			SetDlgItemText(hwnd,IDC_EDITOUTVIEW,"-listen");
			SetDlgItemText(hwnd,IDC_EDITREP,"repeater");
			SetDlgItemInt(hwnd,IDC_EDITID,nummer,false);
			SetDlgItemText(hwnd,IDC_EDITSPORT,"5500");
			SetDlgItemText(hwnd,IDC_EDITVPORT,"5901");

			SetWindowText(GetDlgItem(hwnd, IDC_LOCALIP),strLocalIP);
			SetWindowText(GetDlgItem(hwnd, IDC_REMOTEIP),strExternIP);
			SendMessage(GetDlgItem(hwnd, IDC_RADIOLAN),BM_SETCHECK,TRUE,0);

			EnableWindow(GetDlgItem(hwnd, IDC_RADIOINTERNET),true);
			EnableWindow(GetDlgItem(hwnd, IDC_RADIOLAN), true);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKUSESERVICE), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKRECON), false);
			EnableWindow(GetDlgItem(hwnd, IDC_CHECKREP), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), false);

			EnableWindow(GetDlgItem(hwnd, IDC_EDITID), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), false);

			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTSERV), false);
			EnableWindow(GetDlgItem(hwnd, IDC_EDITOUTVIEW), false);

			EnableWindow(GetDlgItem(hwnd, IDC_CREATESFX), false);
			
			return TRUE;
		}
	
	case WM_COMMAND: 
		switch (LOWORD(wParam)) 
		{
		case IDC_CHECKREP:
			if (SendDlgItemMessage(hwnd, IDC_CHECKREP, BM_GETCHECK, 0, 0))
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITID), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), true);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), true);
			}
			else
			{
				EnableWindow(GetDlgItem(hwnd, IDC_EDITREP), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITID), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITSPORT), false);
				EnableWindow(GetDlgItem(hwnd, IDC_EDITVPORT), false);
			}
			GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
			GetDlgItemText(hwnd, IDC_EDITID, strid, 256);

		case IDC_EDITSPORT:
		case IDC_EDITVPORT:
		case IDC_EDITREP:
		case IDC_LOCALIP:
		case IDC_REMOTEIP:
		case IDC_EDITID:
			if (set) break;
			GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
			GetWindowText(GetDlgItem(hwnd, IDC_EDITSPORT),Serverport,10);
		    GetWindowText(GetDlgItem(hwnd, IDC_EDITVPORT),Viewerport,10);
			GetDlgItemText(hwnd, IDC_REMOTEIP, strExternIP, 256);
			GetDlgItemText(hwnd, IDC_LOCALIP, strLocalIP, 256);
			GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
		case IDC_CHECKRECON:
		case IDC_CHECKUSEENCRYPTION:
			{
				char temp[10];
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKREP), BM_GETCHECK, 0, 0)== BST_CHECKED)
				{
					strcpy(viewercmd,"-autoreconnect 3");
					if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
						{
							strcat(viewercmd," -dsmplugin MSRC4Plugin.dsm");
						}	
							strcat(viewercmd," -proxy ");
							strcat(viewercmd,strrepeater);
							strcat(viewercmd,":");
							strcat(viewercmd,Viewerport);
							strcat(viewercmd," ID");
							strcat(viewercmd,":");
							strcat(viewercmd,strid);
				}
				else
				{
					strcpy(viewercmd,"-listen ");
					strcat(viewercmd,_itoa(testport,temp,10));
					strcat(viewercmd," -autoreconnect 3");
					if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
							{
								strcat(viewercmd," -dsmplugin MSRC4Plugin.dsm");

							}
				}
				SetDlgItemText(hwnd,IDC_EDITOUTVIEW,viewercmd);
			}
		case IDC_RADIOLAN:
		case IDC_RADIOINTERNET:
			if (SendMessage(GetDlgItem(hwnd, IDC_RADIOINTERNET), BM_GETCHECK, 0, 0)== BST_CHECKED)
			{
				char temp[10];
				bool autore;
				bool rep;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKRECON), BM_GETCHECK, 0, 0)== BST_CHECKED)
					autore=true;
				else autore=false;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKREP), BM_GETCHECK, 0, 0)== BST_CHECKED)
				rep=true;
				else rep=false;

				if (!rep && autore)
					{
						strcpy(servercmd,"-autoreconnect -connect ");
						strcat(servercmd,strExternIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if (!rep && !autore)
					{
						strcpy(servercmd,"-connect ");
						strcat(servercmd,strExternIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if( rep && autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-autoreconnect -ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}
				else if(rep && !autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
					{
						strcat(servercmd," -dsmplugin MSRC4Plugin.dsm");
					}
				strcat(servercmd," -run");
			}
			else
			{
				char temp[10];
				bool autore;
				bool rep;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKRECON), BM_GETCHECK, 0, 0)== BST_CHECKED)
					autore=true;
				else autore=false;
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKREP), BM_GETCHECK, 0, 0)== BST_CHECKED)
				rep=true;
				else rep=false;

				if (!rep && autore)
					{
						strcpy(servercmd,"-autoreconnect -connect ");
						strcat(servercmd,strLocalIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if (!rep && !autore)
					{
						strcpy(servercmd,"-connect ");
						strcat(servercmd,strLocalIP);
						strcat(servercmd,":");
						strcat(servercmd,_itoa(testport,temp,10));
					}
				else if( rep && autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-autoreconnect -ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}
				else if(rep && !autore)
					{
						GetDlgItemText(hwnd, IDC_EDITREP, strrepeater, 256);
						GetDlgItemText(hwnd, IDC_EDITID, strid, 256);
						strcpy(servercmd,"-ID:");
						strcat(servercmd,strid);
						strcat(servercmd," -connect ");
						strcat(servercmd,strrepeater);
						strcat(servercmd,":");
						strcat(servercmd,Serverport);
					}
				if (SendMessage(GetDlgItem(hwnd, IDC_CHECKUSEENCRYPTION), BM_GETCHECK, 0, 0)== BST_CHECKED)
					{
						strcat(servercmd," -dsmplugin MSRC4Plugin.dsm");
					}
				strcat(servercmd," -run");
			}
			SetDlgItemText(hwnd,IDC_EDITOUTSERV,servercmd);
			break;
		case IDOK:
			break;
		case IDC_NEXT:
			ShowWindow(hTab6dialog, SW_HIDE);
			ShowWindow(hTab7dialog, SW_SHOW);
			SetFocus(hTab7dialog);
			break;
		case IDC_BACK:
			ShowWindow(hTab7dialog, SW_HIDE);
			ShowWindow(hTab6dialog, SW_SHOW);
			SetFocus(hTab6dialog);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		case IDC_CREATESFX:
			createsfx3(servercmd,viewercmd);
			break;
		}
		return 0;	
	}

	return 0;
}