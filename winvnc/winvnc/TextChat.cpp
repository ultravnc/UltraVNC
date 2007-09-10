/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
// http://ultravnc.sourceforge.net/

#include "stdhdrs.h"
#include "resource.h"
#include "vncclient.h"
#include "vncserver.h"
#include "TextChat.h"
#include "commctrl.h"
#include "richedit.h"

#include "localization.h" // Act : add localization on messages

#define TEXTMAXSIZE 16384
#define MAXNAMESIZE	128 // MAX_COMPUTERNAME_LENGTH+1 (32)
#define CHAT_OPEN  -1  // Todo; put these codes in rfbproto.h
#define CHAT_CLOSE -2
#define CHAT_FINISHED -3

extern HINSTANCE	hAppInstance;

//
//
//
TextChat::TextChat(vncClient *pCC)
{
	m_hDlg=NULL;
	m_pCC = pCC;
	m_fTextChatRunning = false;
	
	m_fPersistentTexts = false;
	m_szLocalText = new char [TEXTMAXSIZE]; // Text (message) entered by local user
	memset(m_szLocalText, 0, TEXTMAXSIZE);
	m_szLastLocalText = new char [TEXTMAXSIZE]; // Last local text (no more necessary)
	memset(m_szLastLocalText, 0, TEXTMAXSIZE);
	m_szRemoteText = new char [TEXTMAXSIZE]; // Incoming remote text (remote message)
	memset(m_szRemoteText, 0, TEXTMAXSIZE);
	m_szRemoteName = new char[MAXNAMESIZE]; // Name of remote machine
	memset(m_szRemoteName,0,MAXNAMESIZE);
	m_szLocalName =  new char[MAXNAMESIZE]; // Name of local machine
	memset(m_szLocalName,0,MAXNAMESIZE);

	m_szTextBoxBuffer = new char[TEXTMAXSIZE]; // History Text (containing all chat messages from everybody)
	memset(m_szTextBoxBuffer,0,TEXTMAXSIZE);

	// memset(m_szUserID, 0, 16);	// Short User ID (name, pseudo...whatever) that replace 
								// the local computer name/IP

	unsigned long pcSize=MAXNAMESIZE;
	if (GetComputerName(m_szLocalName, &pcSize))
	{
		if( pcSize >= MAXNAMESIZE)
		{
			m_szLocalName[MAXNAMESIZE-4]='.';
			m_szLocalName[MAXNAMESIZE-3]='.';
			m_szLocalName[MAXNAMESIZE-2]='.';
			m_szLocalName[MAXNAMESIZE-1]=0x00;
		}
	}
	
	//  Load the Rich Edit control DLL
	m_hRichEdit = LoadLibrary( "RICHED32.DLL" );
	if (!m_hRichEdit)
	{  
		MessageBox( NULL, sz_ID_RICHED32_UNLOAD,
					sz_ID_RICHED32_DLL_LD, MB_OK | MB_ICONEXCLAMATION );
		// Todo: do normal edit instead (no colors)
	}
}


//
//
//
TextChat::~TextChat()
{
	if (m_szLocalText != NULL) delete [] m_szLocalText;
	if (m_szLastLocalText != NULL) delete [] m_szLastLocalText;
	if (m_szRemoteText != NULL) delete [] m_szRemoteText;
	if (m_szRemoteName != NULL) delete [] m_szRemoteName;
	if (m_szLocalName != NULL) delete [] m_szLocalName;
	if (m_szTextBoxBuffer != NULL) delete [] m_szTextBoxBuffer;

	m_fTextChatRunning = false;
	if (m_hDlg!=NULL) SendMessage(m_hDlg, WM_COMMAND, IDCANCEL, 0L);

	if (m_hRichEdit != NULL) FreeLibrary(m_hRichEdit);
}


//
//
//
void TextChat::ShowChatWindow(bool fVisible)
{
	ShowWindow(m_hDlg, fVisible ? SW_RESTORE : SW_MINIMIZE);
	SetForegroundWindow(m_hDlg);
	//SetFocus(GetDlgItem(m_hDlg, IDC_INPUTAREA_EDIT));
}


//
// Set text format to a selection in the Chat area
//
void TextChat::SetTextFormat(bool bBold /*= false */, bool bItalic /*= false*/
	, long nSize /*= 0x75*/, const char* szFaceName /*= "MS Sans Serif"*/, DWORD dwColor /*= BLACK*/)
{
	if ( GetDlgItem( m_hDlg, IDC_CHATAREA_EDIT ) )  //  Sanity Check
	{		
		CHARFORMAT cf;
		memset( &cf, 0, sizeof(CHARFORMAT) ); //  Initialize structure

		cf.cbSize = sizeof(CHARFORMAT);             
		cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
		if (bBold) {
			cf.dwMask |= CFM_BOLD;
			cf.dwEffects |= CFE_BOLD;
		}
		if (bItalic) {
			cf.dwMask |= CFM_ITALIC;  
			cf.dwEffects |= CFE_ITALIC;
		}
		cf.crTextColor = dwColor;					// set color in AABBGGRR mode (alpha-RGB)
		cf.yHeight = nSize;							// set size in points
		strcpy( cf.szFaceName, szFaceName);
													
		SendDlgItemMessage( m_hDlg, IDC_CHATAREA_EDIT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
	}
}

//
//
//
void TextChat::ProcessTextChatMsg(int nTO)
{
	rfbTextChatMsg tcm;
	m_pCC->m_socket->ReadExact(((char *) &tcm) + nTO, sz_rfbTextChatMsg - nTO);
	int len = Swap32IfLE(tcm.length);
	
	if (len == CHAT_OPEN)
	{
		if (m_fTextChatRunning) return;
		// PostMessage(m_pCC->m_server->GetDesktopPointer()->Window(), WM_USER+888, 0, (LPARAM)this);
		DisplayTextChat();
		return;
	}
	else if (len == CHAT_CLOSE)
	{
		// Close TextChat Dialog
		if (!m_fTextChatRunning) return;
		PostMessage(m_hDlg, WM_COMMAND, IDOK, 0);
		return;
	}
	else if (len == CHAT_FINISHED)
	{
		// Close TextChat Dialog
		if (!m_fTextChatRunning) return;
		// m_fTextChatRunning = false;
		// PostMessage(m_hDlg, WM_COMMAND, IDCANCEL, 0);
		return;
	}
	else
	{
		// Read the incoming text
		if (len > TEXTMAXSIZE) return; // Todo: Improve this...
		if (len == 0)
		{
			SetDlgItemText(m_hDlg, IDC_CHATAREA_EDIT, "");
			memset(m_szRemoteText, 0, TEXTMAXSIZE);
		}
		else
		{
			memset(m_szRemoteText, 0, TEXTMAXSIZE);
			m_pCC->m_socket->ReadExact(m_szRemoteText, len);
			ShowChatWindow(true); // Show the chat window on new message reception
			PrintMessage(m_szRemoteText, m_szRemoteName, RED);
		}
	}
}


//
// Tell the client to initiate a TextChat
//
void TextChat::OrderTextChat()
{
	SendTextChatRequest(CHAT_OPEN);
	return;
}


//
//
//
void TextChat::SendTextChatRequest(int nMsg)
{
	if (m_pCC->m_fFileTransferRunning) return; // Don't break a running file transfer please...
    rfbTextChatMsg tcm;
    tcm.type = rfbTextChat;
	tcm.length = Swap32IfLE(nMsg);
    m_pCC->m_socket->SendExact((char *)&tcm, sz_rfbTextChatMsg, rfbTextChat);
	return;
}


//
// Output messages in the chat area 
//
//
void TextChat::PrintMessage(const char* szMessage,const char* szSender,DWORD dwColor /*= BLACK*/)
{
	// char szTextBoxBuffer[TEXTMAXSIZE];			
	// memset(szTextBoxBuffer,0,TEXTMAXSIZE);			
	CHARRANGE cr;	
	
	// sf@2005 - Empty the RichEdit control when it's close to the TEXTMAXSIZE limit
	// In worst case, the RichEdit can contains 2*TEXTMAXSIZE + NAMEMAXSIZE + 4 - 32 bytes
	GETTEXTLENGTHEX lpG;
	lpG.flags = GTL_NUMBYTES;
	lpG.codepage = CP_ACP; // ANSI

	int nLen = SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_GETTEXTLENGTHEX, (WPARAM)&lpG, 0L);
	if (nLen + 32 > TEXTMAXSIZE )
	{
		memset(m_szTextBoxBuffer, 0, TEXTMAXSIZE);
		strcpy(m_szTextBoxBuffer, "------------------------------------------------------------------------------------------------------------------------\n");
		SetDlgItemText(m_hDlg, IDC_CHATAREA_EDIT, m_szTextBoxBuffer);
	}

	// Todo: test if chat text + sender + message > TEXTMAXSIZE -> Modulo display
	if (szSender != NULL && strlen(szSender) > 0) //print the sender's name
	{
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT,WM_GETTEXT, TEXTMAXSIZE-1,(LONG)m_szTextBoxBuffer);
		cr.cpMax = nLen; //strlen(m_szTextBoxBuffer);	 // Select the last caracter to make the text insertion
		cr.cpMin  = cr.cpMax;
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT,EM_EXSETSEL,0,(LONG) &cr);
		SetTextFormat(false,false,0x75,"MS Sans Serif",dwColor);
		_snprintf(m_szTextBoxBuffer, MAXNAMESIZE-1 + 4, "<%s>: ", szSender);		
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT,EM_REPLACESEL,FALSE,(LONG)m_szTextBoxBuffer); // Replace the selection with the message
	}

	nLen = SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_GETTEXTLENGTHEX, (WPARAM)&lpG, 0L);
	if (szMessage != NULL) //print the sender's message
	{	
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT,WM_GETTEXT, TEXTMAXSIZE-1,(LONG)m_szTextBoxBuffer);
		cr.cpMax = nLen; //strlen(m_szTextBoxBuffer);	 // Select the last caracter to make the text insertion
		cr.cpMin  = cr.cpMax;
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT,EM_EXSETSEL,0,(LONG) &cr);
		SetTextFormat(false, false, 0x75, "MS Sans Serif", dwColor != GREY ? BLACK : GREY);	
		_snprintf(m_szTextBoxBuffer, TEXTMAXSIZE-1, "%s", szMessage);		
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT,EM_REPLACESEL,FALSE,(LONG)m_szTextBoxBuffer); 
	}

	// Scroll down the chat window
	// The following seems necessary under W9x & Me if we want the Edit to scroll to bottom...
	SCROLLINFO si;
    ZeroMemory(&si, sizeof(SCROLLINFO));
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE|SIF_PAGE;
    GetScrollInfo(GetDlgItem(m_hDlg, IDC_CHATAREA_EDIT), SB_VERT, &si);
	si.nPos = si.nMax - max(si.nPage - 1, 0);
	SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, si.nPos), 0L);	// Scroll down the ch

	// This line does the bottom scrolling correctly under NT4,W2K, XP...
	// SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, WM_VSCROLL, SB_BOTTOM, 0L);

}

//
// Send local text content
//
void TextChat::SendLocalText(void)
{
	if (m_pCC->m_fFileTransferRunning) return; // Don't break a running file transfer please...

	// We keep it because we could use it
	// for future retype functionality. (F3)
	memcpy(m_szLastLocalText, m_szLocalText, strlen(m_szLocalText));

	PrintMessage(m_szLocalText, m_szLocalName, BLUE);

    rfbTextChatMsg tcm;
    tcm.type = rfbTextChat;
	tcm.length = Swap32IfLE(strlen(m_szLocalText));
    m_pCC->m_socket->SendExact((char *)&tcm, sz_rfbTextChatMsg, rfbTextChat);
	m_pCC->m_socket->SendExact((char *)m_szLocalText, strlen(m_szLocalText));

	//and we clear the input box
	SetDlgItemText(m_hDlg, IDC_INPUTAREA_EDIT, "");
	return;
}



LRESULT CALLBACK TextChat::DoDialogThread(LPVOID lpParameter)
{
	TextChat* _this = (TextChat*)lpParameter;

	_this->m_fTextChatRunning = true;
	// TODO: Place code here.
	HDESK desktop;
	//vnclog.Print(LL_INTERR, VNCLOG("SelectDesktop \n"));
	//vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop2 NULL\n"));
	desktop = OpenInputDesktop(0, FALSE,
								DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
								DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
								DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
								DESKTOP_SWITCHDESKTOP | GENERIC_WRITE
								);

	if (desktop == NULL)
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop Error \n"));
	else 
		vnclog.Print(LL_INTERR, VNCLOG("OpenInputdesktop OK\n"));

	HDESK old_desktop = GetThreadDesktop(GetCurrentThreadId());
	DWORD dummy;

	char new_name[256];

	if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy))
	{
		vnclog.Print(LL_INTERR, VNCLOG("!GetUserObjectInformation \n"));
	}

	vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK to %s (%x) from %x\n"), new_name, desktop, old_desktop);

	if (!SetThreadDesktop(desktop))
	{
		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK:!SetThreadDesktop \n"));
	}

	if (!CloseDesktop(old_desktop))
		vnclog.Print(LL_INTERR, VNCLOG("SelectHDESK failed to close old desktop %x (Err=%d)\n"), old_desktop, GetLastError());

 	 DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_TEXTCHAT_DLG), 
							NULL, (DLGPROC) TextChatDlgProc, (LONG) _this);
	 CloseDesktop(desktop);
	 return 0;
}


/*int TextChat::DoDialog()
{
	m_fTextChatRunning = true; // Here.Important.
 	return DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_TEXTCHAT_DLG), 
							NULL, (DLGPROC) TextChatDlgProc, (LONG) this);
}*/

HWND TextChat::DisplayTextChat()
{
	DWORD threadID;
	HANDLE m_Thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)(TextChat::DoDialogThread),(LPVOID)this, 0, &threadID);
	ResumeThread(m_Thread);
	return (HWND)0;

}

//
//
//
void TextChat::KillDialog()
{
	// DestroyWindow(m_hDlg);
	m_fTextChatRunning = false;
	PostMessage(m_hDlg, WM_COMMAND, IDCANCEL, 0);
}


//
//
//
void TextChat::RefuseTextChat()
{
	SendTextChatRequest(CHAT_CLOSE);
	SendTextChatRequest(CHAT_FINISHED);
}


//
//
//
BOOL CALLBACK TextChat::TextChatDlgProc(  HWND hWnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam ) {

	TextChat *_this = (TextChat *) GetWindowLong(hWnd, GWL_USERDATA);

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
            SetWindowLong(hWnd, GWL_USERDATA, lParam);
            TextChat *_this = (TextChat *) lParam;

			if (_this->m_szLocalText == NULL || _this->m_szRemoteText == NULL)
				EndDialog(hWnd, FALSE);

			_this->m_hWnd = hWnd;
			_this->m_hDlg = hWnd;

			if (_snprintf(_this->m_szRemoteName,MAXNAMESIZE-1,"%s", _this->m_pCC->GetClientName()) < 0 )
			{
				_this->m_szRemoteName[MAXNAMESIZE-4]='.';
				_this->m_szRemoteName[MAXNAMESIZE-3]='.';
				_this->m_szRemoteName[MAXNAMESIZE-2]='.';
				_this->m_szRemoteName[MAXNAMESIZE-1]=0x00;
			}	

			const long lTitleBufSize = 256;			
			char szTitle[lTitleBufSize];
			
			_snprintf(szTitle,lTitleBufSize-1, sz_ID_CHAT_WITH_S_ULTRAVNC,_this->m_szRemoteName);
			SetWindowText(hWnd, szTitle);			

			memset(_this->m_szLocalText, 0, TEXTMAXSIZE);

			// Local message box	
			SetDlgItemText(hWnd, IDC_INPUTAREA_EDIT, _this->m_szLocalText); 
			
			//  Chat area			
			_this->SetTextFormat(); //  Set character formatting and background color
			SendDlgItemMessage( hWnd, IDC_CHATAREA_EDIT, EM_SETBKGNDCOLOR, FALSE, 0xFFFFFF ); 

			// if (!_this->m_fPersistentTexts)
			{
				memset(_this->m_szLastLocalText, 0, TEXTMAXSIZE); // Future retype functionnality
				memset(_this->m_szTextBoxBuffer, 0, TEXTMAXSIZE); // Clear Chat area 
				//  Load and display diclaimer message
				// sf@2005 - Only if the DSMplugin is used
				if (!_this->m_pCC->m_server->GetDSMPluginPointer()->IsEnabled())
				{
					if (LoadString(hAppInstance,IDS_WARNING,_this->m_szRemoteText, TEXTMAXSIZE -1) )
						_this->PrintMessage(_this->m_szRemoteText, NULL ,GREY);
				}
			}
			// else
			//	SendDlgItemMessage(hWnd,IDC_CHATAREA_EDIT, WM_SETTEXT, 0, (LONG)_this->m_szTextBoxBuffer);
			
			// Scroll down the chat window
			// The following seems necessary under W9x & Me if we want the Edit to scroll to bottom...
			SCROLLINFO si;
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE|SIF_PAGE;
			GetScrollInfo(GetDlgItem(hWnd, IDC_CHATAREA_EDIT), SB_VERT, &si);
			si.nPos = si.nMax - max(si.nPage - 1, 0);
			SendDlgItemMessage(hWnd, IDC_CHATAREA_EDIT, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, si.nPos), 0L);	
			// This line does the bottom scrolling correctly under NT4,W2K, XP...
			// SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, WM_VSCROLL, SB_BOTTOM, 0L);

			// SendDlgItemMessage(hWnd, IDC_PERSISTENT_CHECK, BM_SETCHECK, _this->m_fPersistentTexts, 0);

			//if (strlen(_this->m_szUserID) > 0)
			//	SetDlgItemText(hWnd, IDC_USERID_EDIT, _this->m_szUserID);
			
			SetForegroundWindow(hWnd);
            return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/*
		case IDC_PERSISTENT_CHECK:
			  _this->m_fPersistentTexts = (SendDlgItemMessage(hWnd, IDC_PERSISTENT_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return TRUE;
		*/

		case IDOK:
			// Client order to close TextChat 			
			EndDialog(hWnd, FALSE);
			_this->m_fTextChatRunning = false;
			_this->SendTextChatRequest(CHAT_FINISHED);
			return TRUE;

		case IDCANCEL:			
			_this->SendTextChatRequest(CHAT_CLOSE); // Client must close TextChat
			EndDialog(hWnd, FALSE);
			_this->m_fTextChatRunning = false;
			_this->SendTextChatRequest(CHAT_FINISHED);
			return TRUE;

		case IDC_SEND_B:
			{
			memset(_this->m_szLocalText,0,TEXTMAXSIZE);
			UINT nRes = GetDlgItemText( hWnd, IDC_LOCALTEXT_EDIT, _this->m_szLocalText, TEXTMAXSIZE-1);
			strcat(_this->m_szLocalText, "\n");
			_this->SendLocalText();	
			SetFocus(GetDlgItem(hWnd, IDC_INPUTAREA_EDIT));
			}
			return TRUE;

		case IDC_INPUTAREA_EDIT:
			if(HIWORD(wParam) == EN_UPDATE)			
			{
				UINT nRes = GetDlgItemText( hWnd, IDC_INPUTAREA_EDIT, _this->m_szLocalText, TEXTMAXSIZE);
				if (strstr(_this->m_szLocalText, "\n") > 0 ) // Enter triggers the message transmission
				{
					// nRes = GetDlgItemText( hWnd, IDC_USERID_EDIT, _this->m_szUserID, 16);
					_this->SendLocalText();
				}								
			}
			return TRUE;

		case IDC_HIDE_B:
			_this->ShowChatWindow(false);
			return TRUE;

		default:
			return TRUE;
			break;
		}
		break;

	case WM_SYSCOMMAND:
		switch (LOWORD(wParam))
		{
		case SC_RESTORE:
			_this->ShowChatWindow(true);
			//SetFocus(GetDlgItem(hWnd, IDC_INPUTAREA_EDIT));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		// _this->m_fTextChatRunning = false;
		// _this->SendTextChatRequest(CHAT_FINISHED);
		EndDialog(hWnd, FALSE);
		return TRUE;
	}
	return 0;
}




