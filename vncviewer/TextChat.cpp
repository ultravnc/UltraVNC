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
#include "TextChat.h"
#include "Exception.h"
#include "commctrl.h"
#include "richedit.h"
#include "common/win32_helpers.h"
#include <rdr/Exception.h>
#include "UltraVNCHelperFunctions.h"

using namespace helper;

#define TEXTMAXSIZE 16384
#define MAXNAMESIZE	128 // MAX_COMPUTERNAME_LENGTH+1 (32)
#define CHAT_OPEN  -1 // Todo; put these codes in rfbproto.h
#define CHAT_CLOSE -2
#define CHAT_FINISHED -3

// [v1.0.2-jp1 fix]
LRESULT CALLBACK SBProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LONG_PTR pDefSBProc;
extern HINSTANCE m_hInstResDLL;

extern wchar_t sz_E1[64];
extern wchar_t sz_E2[64];

// Convert network bytes (ANSI or UTF-8) to a wide string
static bool NetBytesToWide(const char* inBytes, int inLen, UINT assumedCodepage, wchar_t* outW, int outCap)
{
	if (!outW || outCap <= 0) return false;
	outW[0] = 0;
	if (!inBytes || inLen <= 0) return true;

	// Validate UTF-8 assumption: fall back to ACP if invalid
	if (assumedCodepage == CP_UTF8) {
		// Quick validation: try conversion with error flag
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, inBytes, inLen, NULL, 0) <= 0)
			assumedCodepage = CP_ACP;
	}

	int n = MultiByteToWideChar(assumedCodepage, 0, inBytes, inLen, outW, outCap - 1);
	if (n <= 0) return false;
	outW[n] = 0;
	return true;
}

// Convert wide string to network bytes (UTF-8 or ACP)
static int WideToNetBytes(const wchar_t* w, UINT codepage, char* out, int outCap)
{
	if (!out || outCap <= 0) return 0;
	out[0] = 0;
	if (!w || !w[0]) return 0;
	int n = WideCharToMultiByte(codepage, 0, w, -1, out, outCap - 1, NULL, NULL);
	if (n <= 0) return 0;
	out[n] = 0;
	return n;
}

//
//
//
TextChat::TextChat(VNCviewerApp *pApp, ClientConnection *pCC)
{
	m_pApp	= pApp;
	m_pCC	= pCC;
	m_fTextChatRunning = false;
	//test m_fVisible always false
	m_fVisible = false;
	
	m_fPersistentTexts = false;
	m_szLocalText    = new wchar_t[TEXTMAXSIZE];
	memset(m_szLocalText,    0, TEXTMAXSIZE * sizeof(wchar_t));
	m_szLastLocalText = new wchar_t[TEXTMAXSIZE];
	memset(m_szLastLocalText, 0, TEXTMAXSIZE * sizeof(wchar_t));
	m_szRemoteText   = new wchar_t[TEXTMAXSIZE];
	memset(m_szRemoteText,   0, TEXTMAXSIZE * sizeof(wchar_t));
	m_szRemoteName   = new wchar_t[MAXNAMESIZE];
	memset(m_szRemoteName,   0, MAXNAMESIZE  * sizeof(wchar_t));
	m_szLocalName    = new wchar_t[MAXNAMESIZE];
	memset(m_szLocalName,    0, MAXNAMESIZE  * sizeof(wchar_t));

	m_szTextBoxBuffer = new wchar_t[TEXTMAXSIZE];
	memset(m_szTextBoxBuffer, 0, TEXTMAXSIZE * sizeof(wchar_t));

	unsigned long pcSize = MAXNAMESIZE;
	GetComputerNameW(m_szLocalName, &pcSize);

	//  Load the Rich Edit control DLL
	m_hRichEdit = LoadLibraryEx( _T("RICHED32.DLL"), NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (!m_hRichEdit)
	{  
		yesUVNCMessageBox(m_hInstResDLL, NULL, sz_E1, sz_E2, MB_ICONEXCLAMATION );
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
	SendMessage(m_hDlg, WM_COMMAND, IDOK, 0L);

	if (m_hRichEdit != NULL) FreeLibrary(m_hRichEdit);

}


//
// Set text format to a selection in the Chat area
//
void TextChat::SetTextFormat(bool bBold /*= false */, bool bItalic /*= false*/
	, long nSize /*= 0x75*/, const wchar_t* szFaceName /*= L"MS Shell Dlg 2"*/, DWORD dwColor /*= BLACK*/)
{
	if ( GetDlgItem( m_hDlg, IDC_CHATAREA_EDIT ) )  //  Sanity Check
	{		
		CHARFORMAT cf;
		memset( &cf, 0, sizeof(CHARFORMAT) ); //  Initialize structure

		cf.cbSize = sizeof(CHARFORMAT);             
		cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
		if (bBold)
		{
			cf.dwMask |= CFM_BOLD;
			cf.dwEffects |= CFE_BOLD;
		}
		if (bItalic)
		{
			cf.dwMask |= CFM_ITALIC;  
			cf.dwEffects |= CFE_ITALIC;
		}
		cf.crTextColor = dwColor;					// set color in AABBGGRR mode (alpha-RGB)
		cf.yHeight = nSize;							// set size in points
		wcscpy_s(cf.szFaceName, 32, szFaceName);
													
		SendDlgItemMessage( m_hDlg, IDC_CHATAREA_EDIT, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf );
	}
}


//
//
//
void TextChat::ProcessTextChatMsg() 
{
	rfbTextChatMsg tcm;
	m_pCC->ReadExact(((char *) &tcm) + m_pCC->m_nTO, sz_rfbTextChatMsg - m_pCC->m_nTO);
	UINT len = Swap32IfLE(tcm.length);
	
	if (len == CHAT_OPEN)
	{
		// Open Text Chat Dialog
		PostMessage(m_pCC->m_hwndMain, WM_SYSCOMMAND, ID_TEXTCHAT, (LPARAM)0L);
		return;
	}
	else if (len == CHAT_CLOSE)
	{
		// Close Text Chat Dialog
		if (!m_fTextChatRunning) return;
		PostMessage(m_hDlg, WM_COMMAND, IDOK, 0);
		return;
	}
	else if (len == CHAT_FINISHED)
	{
		// On Server notif, request FullScreen update
		if (!m_fTextChatRunning) return;
		m_fTextChatRunning = false;
		//adzm 2010-09 - all socket writes must remain on a single thread, but we only need an async request here
		m_pCC->SendAppropriateFramebufferUpdateRequest(true);
		return;
	}
	else
	{
		// Read the incoming text
		if (len > TEXTMAXSIZE) return; // Todo: Improve this...
		if (len == 0)
		{
			SetDlgItemTextW(m_hDlg, IDC_REMOTETEXT_EDIT, L"");
			memset(m_szRemoteText, 0, TEXTMAXSIZE * sizeof(wchar_t));
		}
		else
		{
			char* raw = new char[len + 1];
			raw[len] = 0;
			m_pCC->ReadExact(raw, len);

			memset(m_szRemoteText, 0, TEXTMAXSIZE * sizeof(wchar_t));
			UINT assumed = m_pCC->m_supportsUnicodeTextChat ? CP_UTF8 : CP_ACP;
			if (!NetBytesToWide(raw, (int)len, assumed, m_szRemoteText, TEXTMAXSIZE)) {
				// fallback: force ACP
				NetBytesToWide(raw, (int)len, CP_ACP, m_szRemoteText, TEXTMAXSIZE);
			}
			delete[] raw;

			PrintMessage(m_szRemoteText, m_szRemoteName, RED);
			ShowChatWindow(true); // Show the chat window on new message reception
		}
	}
}


//
//
//
void TextChat::SendTextChatRequest(int nMsg)
{
	if (m_pCC->m_bKillThread) return;
    rfbTextChatMsg tcm;
	memset(&tcm, 0, sizeof(rfbTextChatMsg));
    tcm.type = rfbTextChat;
	tcm.length = Swap32IfLE(nMsg);
    m_pCC->WriteExact((char *)&tcm, sz_rfbTextChatMsg, rfbTextChat);
	return;
}

//
// Output messages in the chat area 
//
//
void TextChat::PrintMessage(const wchar_t* szMessage, const wchar_t* szSender, DWORD dwColor /*= BLACK*/)
{
	CHARRANGE cr;	
	
	GETTEXTLENGTHEX lpG;
	lpG.flags = GTL_NUMBYTES;
	lpG.codepage = CP_UTF8;

	int nLen = SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_GETTEXTLENGTHEX, (WPARAM)&lpG, 0L);
	if (nLen + 32 > TEXTMAXSIZE)
	{
		SetDlgItemTextW(m_hDlg, IDC_CHATAREA_EDIT, L"------------------------------------------------------------------------------------------------------------------------\n");
		nLen = 0;
	}

	// Helper: insert wide text at end of RichEdit using EM_SETTEXTEX with CP_UTF8
	auto InsertWide = [&](const wchar_t* text) {
		char utf8[TEXTMAXSIZE];
		int n = WideCharToMultiByte(CP_UTF8, 0, text, -1, utf8, TEXTMAXSIZE - 1, NULL, NULL);
		if (n <= 0) return;
		utf8[n] = 0;
		SETTEXTEX st;
		st.flags = ST_SELECTION;
		st.codepage = CP_UTF8;
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_SETTEXTEX, (WPARAM)&st, (LPARAM)utf8);
	};

	// Todo: test if chat text + sender + message > TEXTMAXSIZE -> Modulo display
	if (szSender != NULL) //print the sender's name
	{
		cr.cpMax = nLen;
		cr.cpMin = cr.cpMax;
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_EXSETSEL, 0, (LPARAM)&cr);

		// [v1.0.2-jp1 fix]
		SetTextFormat(false, false, 0x75, L"MS Shell Dlg 2", dwColor);

		_snwprintf_s(m_szTextBoxBuffer, TEXTMAXSIZE - 1, _TRUNCATE, L"<%s>: ", szSender);
		InsertWide(m_szTextBoxBuffer);
	}

	nLen = SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_GETTEXTLENGTHEX, (WPARAM)&lpG, 0L);
	if (szMessage != NULL) //print the sender's message
	{	
		cr.cpMax = nLen;
		cr.cpMin = cr.cpMax;
		SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, EM_EXSETSEL, 0, (LPARAM)&cr);

		// [v1.0.2-jp1 fix]
		SetTextFormat(false, false, 0x75, L"MS Shell Dlg 2", dwColor != GREY ? BLACK : GREY);

		_snwprintf_s(m_szTextBoxBuffer, TEXTMAXSIZE - 1, _TRUNCATE, L"%s", szMessage);
		InsertWide(m_szTextBoxBuffer);
	}

	// Scroll down the chat window
	// The following seems necessary under W9x & Me if we want the Edit to scroll to bottom...
	SCROLLINFO si;
    ZeroMemory(&si, sizeof(SCROLLINFO));
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE|SIF_PAGE;
    GetScrollInfo(GetDlgItem(m_hDlg, IDC_CHATAREA_EDIT), SB_VERT, &si);
	si.nPos = si.nMax - maximum(si.nPage - 1, 0);
	SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, si.nPos), 0L);	// Scroll down the ch

	// This line does the bottom scrolling correctly under Windows NT4, Windows 2000, Windows XP...
	// SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, WM_VSCROLL, SB_BOTTOM, 0L);

}

//
// Send local text content
//
void TextChat::SendLocalText(void)
{
	// Keep for future retype functionality (F3)
	wcsncpy_s(m_szLastLocalText, TEXTMAXSIZE, m_szLocalText, _TRUNCATE);

	// Read input as UTF-16 directly from the control
	GetDlgItemTextW(m_hDlg, IDC_INPUTAREA_EDIT, m_szLocalText, TEXTMAXSIZE - 2);
	if (m_szLocalText[0] == 0) {
		SetDlgItemTextW(m_hDlg, IDC_INPUTAREA_EDIT, L"");
		return;
	}
	// Keep original behavior of appending newline
	wcscat_s(m_szLocalText, TEXTMAXSIZE, L"\n");

	PrintMessage(m_szLocalText, m_szLocalName, BLUE);

	// Backward compatible send:
	// - New UltraVNC server: send UTF-8 bytes.
	// - Legacy server: send ACP bytes.
	char sendBytes[TEXTMAXSIZE * 3]; // UTF-8 can be up to 3x wchar count
	UINT sendCp = m_pCC->m_supportsUnicodeTextChat ? CP_UTF8 : CP_ACP;
	int sendLen = WideToNetBytes(m_szLocalText, sendCp, sendBytes, sizeof(sendBytes));
	if (sendLen <= 0 && sendCp != CP_UTF8) {
		// fallback to UTF-8 if ACP conversion failed
		sendLen = WideToNetBytes(m_szLocalText, CP_UTF8, sendBytes, sizeof(sendBytes));
	}

	rfbTextChatMsg tcm;
	memset(&tcm, 0, sizeof(rfbTextChatMsg));
	tcm.type = rfbTextChat;
	tcm.length = Swap32IfLE((CARD32)sendLen);
	//adzm 2010-09
	m_pCC->WriteExactQueue((char *)&tcm, sz_rfbTextChatMsg, rfbTextChat);
	if (sendLen > 0) {
		m_pCC->WriteExact((char *)sendBytes, sendLen);
	}

	//and we clear the input box
	SetDlgItemTextW(m_hDlg, IDC_INPUTAREA_EDIT, L"");
}


//
//
//
int TextChat::DoDialog()
{
	extern HINSTANCE m_hInstResDLL;
 	return DialogBoxParam(m_hInstResDLL, DIALOG_MAKEINTRESOURCE(IDD_TEXTCHAT_DLG), 
							NULL, (DLGPROC) TextChatDlgProc, (LONG_PTR) this);
}


//
//
//
void TextChat::KillDialog()
{
	// DestroyWindow(m_hDlg);
    PostMessage(m_hDlg, WM_COMMAND, MAKEWPARAM(IDCANCEL,0), 0);
}

//
//
//
void TextChat::ShowChatWindow(bool fVisible)
{
	ShowWindow(m_hDlg, fVisible ? SW_RESTORE : SW_MINIMIZE);
	SetForegroundWindow(m_hDlg);
	// Put the Chat Windows always on Top if fullscreen
	if (fVisible && m_pCC->InFullScreenMode())
	{
		RECT Rect;
		GetWindowRect(m_hDlg, &Rect);
		SetWindowPos(m_hDlg, 
					HWND_TOPMOST,
					Rect.left,
					Rect.top,
					Rect.right - Rect.left,
					Rect.bottom - Rect.top,
					SWP_SHOWWINDOW);
	}
	//test m_fVisible always false
	//m_fVisible = fVisible; // This enables screen updates to be processed in ClientConnection
	// Refresh screen view if Chat window has been hidden
	//adzm 2010-09 - all socket writes must remain on a single thread, but we only need an async request here
	if (!fVisible)
		m_pCC->SendAppropriateFramebufferUpdateRequest(true);
}

// [v1.0.2-jp1 fix]
void AdjustLeft(LPRECT lprc)
{
	int cx = lprc->right - lprc->left - GetSystemMetrics(SM_CXSIZEFRAME) * 2;

	if(cx < 240){
		lprc->left = lprc->right - 240 - GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	}
}

void AdjustTop(LPRECT lprc)
{
	int cy = lprc->bottom - lprc->top - GetSystemMetrics(SM_CYSIZEFRAME) * 2;

	if(cy < 179){
		lprc->top = lprc->bottom - 179 - GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	}
}

void AdjustRight(LPRECT lprc)
{
	int cx = lprc->right - lprc->left - GetSystemMetrics(SM_CXSIZEFRAME) * 2;

	if(cx < 240){
		lprc->right = lprc->left + 240 + GetSystemMetrics(SM_CXSIZEFRAME) * 2;
	}
}

void AdjustBottom(LPRECT lprc)
{
	int cy = lprc->bottom - lprc->top - GetSystemMetrics(SM_CYSIZEFRAME) * 2;

	if(cy < 179){
		lprc->bottom = lprc->top + 179 + GetSystemMetrics(SM_CYSIZEFRAME) * 2;
	}
}

//
//
//
BOOL CALLBACK TextChat::TextChatDlgProc(  HWND hWnd,  UINT uMsg,  WPARAM wParam, LPARAM lParam )
{
    TextChat *_this = helper::SafeGetWindowUserData<TextChat>(hWnd);

	switch (uMsg)
	{

	case WM_INITDIALOG:
		{
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_TRAY));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            helper::SafeSetWindowUserData(hWnd, lParam);
            TextChat *_this = (TextChat *) lParam;
			if (_this->m_szLocalText == NULL || _this->m_szRemoteText == NULL)
				EndDialog(hWnd, FALSE);

			// Window always on top if Fullscreen On
			if (_this->m_pCC->InFullScreenMode())
			{
				RECT Rect;
				GetWindowRect(hWnd, &Rect);
				SetWindowPos(hWnd, 
							HWND_TOPMOST,
							Rect.left,
							Rect.top,
							Rect.right - Rect.left,
							Rect.bottom - Rect.top,
							SWP_SHOWWINDOW);
			}

            // CentreWindow(hWnd);
			_this->m_hWnd = hWnd;
			_this->m_hDlg = hWnd;
			
			wcsncpy_s(_this->m_szRemoteName, MAXNAMESIZE, _this->m_pCC->m_desktopName, _TRUNCATE);

			wchar_t szTitle[256] = {};
			_snwprintf_s(szTitle, 256, _TRUNCATE, L"UltraVNC Viewer - Chat with <%s>", _this->m_szRemoteName);
			SetWindowTextW(hWnd, szTitle);

			// Trunc the remote name for display in Chat Area before the first parenthesis, if any.
			wchar_t *p = wcschr(_this->m_szRemoteName, L'(');
			if (p != NULL) *(p - 1) = L'\0';

			//  Chat area			
			_this->SetTextFormat(); //  Set character formatting and background color
			SendDlgItemMessage( hWnd, IDC_CHATAREA_EDIT, EM_SETBKGNDCOLOR, FALSE, 0xFFFFFF ); 

			memset(_this->m_szLocalText, 0, TEXTMAXSIZE * sizeof(wchar_t));
			// if (!_this->m_fPersistentTexts)
			{
				memset(_this->m_szLastLocalText, 0, TEXTMAXSIZE * sizeof(wchar_t));
				memset(_this->m_szTextBoxBuffer, 0, TEXTMAXSIZE * sizeof(wchar_t)); // Clear Chat area 
				 //  Load and display diclaimer message
				// sf@2005 - Only if the DSMplugin is used
				if (_this->m_szRemoteText)
				if (!_this->m_pCC->m_fUsePlugin)
				{
					if (LoadStringW(m_hInstResDLL, IDS_WARNING, _this->m_szRemoteText, TEXTMAXSIZE - 1))
				{
					_this->PrintMessage(_this->m_szRemoteText, NULL, GREY);
				}
				}
			}

			if (_this->m_szLocalText)    SetDlgItemTextW(hWnd, IDC_INPUTAREA_EDIT, _this->m_szLocalText);
			if (_this->m_szTextBoxBuffer) SetDlgItemTextW(hWnd, IDC_CHATAREA_EDIT,  _this->m_szTextBoxBuffer);

			// Scroll down the chat window
			// The following seems necessary under W9x & Me if we want the Edit to scroll to bottom...
			SCROLLINFO si;
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_RANGE|SIF_PAGE;
			GetScrollInfo(GetDlgItem(hWnd, IDC_CHATAREA_EDIT), SB_VERT, &si);
			si.nPos = si.nMax - maximum(si.nPage - 1, 0);
			SendDlgItemMessage(hWnd, IDC_CHATAREA_EDIT, WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, si.nPos), 0L);	
			// This line does the bottom scrolling correctly under Windows NT4, Windows 2000, Windows XP...
			// SendDlgItemMessage(m_hDlg, IDC_CHATAREA_EDIT, WM_VSCROLL, SB_BOTTOM, 0L);

			// SendDlgItemMessage(hWnd, IDC_PERSISTENT_CHECK, BM_SETCHECK, _this->m_fPersistentTexts, 0);

			// Tell the other side to open the Text Chat Window
			_this->SendTextChatRequest(CHAT_OPEN);

			SetForegroundWindow(hWnd);
			
			// [v1.0.2-jp1 fix] SUBCLASS Split bar
            pDefSBProc = helper::SafeGetWindowProc(GetDlgItem(hWnd, IDC_STATIC_SPLIT));
            helper::SafeSetWindowProc(GetDlgItem(hWnd, IDC_STATIC_SPLIT), (LONG_PTR)SBProc);

			return TRUE;
		}
		// break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		/*
		case IDC_PERSISTENT_CHECK:
		    _this->m_fPersistentTexts = (SendDlgItemMessage(hWnd, IDC_PERSISTENT_CHECK, BM_GETCHECK, 0, 0) == BST_CHECKED);
			return TRUE;
		*/

		case IDOK:
			// Server orders to close Text Chat 			

			// [v1.0.2-jp1 fix] UNSUBCLASS Split bar
            helper::SafeSetWindowProc(GetDlgItem(hWnd, IDC_STATIC_SPLIT), pDefSBProc);

			EndDialog(hWnd, FALSE);
			return TRUE;

		case IDCANCEL:			
			_this->SendTextChatRequest(CHAT_CLOSE); // Server must close Text Chat

			// [v1.0.2-jp1 fix] UNSUBCLASS Split bar
            helper::SafeSetWindowProc(GetDlgItem(hWnd, IDC_STATIC_SPLIT), pDefSBProc);

			EndDialog(hWnd, FALSE);
			return TRUE;

		case IDC_SEND_B:
			_this->SendLocalText();
			SetFocus(GetDlgItem(hWnd, IDC_INPUTAREA_EDIT));
			return TRUE;

		case IDC_HIDE_B:
			_this->ShowChatWindow(false);
			return TRUE;

		case IDC_INPUTAREA_EDIT:
			if (HIWORD(wParam) == EN_UPDATE)
			{
				GetDlgItemTextW(hWnd, IDC_INPUTAREA_EDIT, _this->m_szLocalText, TEXTMAXSIZE - 1);
				if (wcschr(_this->m_szLocalText, L'\n') != nullptr)
				{
					_this->SendLocalText();
				}
			}
			return TRUE;
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

	// [v1.0.2-jp1 fix-->]
	case WM_SIZING:
		LPRECT lprc;
		lprc = (LPRECT)lParam;
		switch(wParam){
		case WMSZ_TOPLEFT:
			AdjustTop(lprc);
			AdjustLeft(lprc);
		case WMSZ_TOP:
			AdjustTop(lprc);
		case WMSZ_TOPRIGHT:
			AdjustTop(lprc);
			AdjustRight(lprc);
		case WMSZ_LEFT:
			AdjustLeft(lprc);
		case WMSZ_RIGHT:
			AdjustRight(lprc);
		case WMSZ_BOTTOMLEFT:
			AdjustBottom(lprc);
			AdjustLeft(lprc);
		case WMSZ_BOTTOM:
			AdjustBottom(lprc);
		case WMSZ_BOTTOMRIGHT:
			AdjustBottom(lprc);
			AdjustRight(lprc);
		}
		return TRUE;
	case WM_SIZE:
		int cx;
		int cy;
		int icy;
		RECT rc;

		if(wParam == SIZE_MINIMIZED){
			break;
		}

		cx = LOWORD(lParam);
		cy = HIWORD(lParam);
		GetWindowRect(GetDlgItem(hWnd, IDC_INPUTAREA_EDIT), &rc);
		icy = rc.bottom - rc.top;
		if(cy - icy - 12 < 80){
			icy = cy - 92;
		}
		MoveWindow(GetDlgItem(hWnd, IDC_CHATAREA_EDIT),  4,             4, cx -  8, cy - icy - 16, TRUE); 
		MoveWindow(GetDlgItem(hWnd, IDC_STATIC_SPLIT),   4, cy - icy - 12, cx -  8,             8, TRUE); 
		MoveWindow(GetDlgItem(hWnd, IDC_INPUTAREA_EDIT), 4, cy - icy -  4, cx - 88,           icy, TRUE);

		MoveWindow(GetDlgItem(hWnd, IDC_SEND_B), cx - 76, cy - 64, 72, 20, TRUE); 
		MoveWindow(GetDlgItem(hWnd, IDC_HIDE_B), cx - 76, cy - 40, 72, 18, TRUE); 
		MoveWindow(GetDlgItem(hWnd, IDCANCEL),   cx - 76, cy - 22, 72, 18, TRUE);

		InvalidateRect(hWnd, NULL, FALSE);
		return TRUE;
	// [<--v1.0.2-jp1 fix]

	case WM_DESTROY:
		// _this->SendTextChatRequest(_this, CHAT_FINISHED);
		EndDialog(hWnd, FALSE);
		_this->m_fTextChatRunning = false;
		return TRUE;
	}
	return 0;
}

// [JEE6 Fix] Split bar
void DrawResizeLine(HWND hWnd, int y)
{
	HWND hParent;
	RECT rc;
	HDC hDC;

	hParent = GetParent(hWnd);
	GetClientRect(hParent, &rc);

	if(y < 80){
		y = 80;
	}
	else if(y > rc.bottom - 80){
		y = rc.bottom - 80;
	}
	
	hDC = GetDC(hParent);
	SetROP2(hDC, R2_NOTXORPEN);
	MoveToEx(hDC, rc.left, y, NULL);
	LineTo(hDC, rc.right, y);
	MoveToEx(hDC, rc.left, y+1, NULL);
	LineTo(hDC, rc.right, y+1);
	MoveToEx(hDC, rc.left, y+2, NULL);
	LineTo(hDC, rc.right, y+2);
	MoveToEx(hDC, rc.left, y+3, NULL);
	LineTo(hDC, rc.right, y+3);
	ReleaseDC(hParent, hDC);
}

// [v1.0.2-jp1 fix] Split bar proc
LRESULT CALLBACK SBProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bCapture;
	static UINT u;
	static int oldy;
	HWND hParent;
	RECT rc;
	POINT cp;
	int y;
	int cx;
	int cy;

	switch(uMsg){
	case WM_SETCURSOR:
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
		return TRUE;
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		bCapture = TRUE;
		u = GetCaretBlinkTime();
		SetCaretBlinkTime(0x7fffffff);
		GetCursorPos(&cp);
		hParent = GetParent(hWnd);
		ScreenToClient(hParent, &cp);
		DrawResizeLine(hWnd, cp.y);
		oldy = cp.y;
		break;
	case WM_MOUSEMOVE:
		if(bCapture){
			GetCursorPos(&cp);
			hParent = GetParent(hWnd);
			ScreenToClient(hParent, &cp);
			DrawResizeLine(hWnd, oldy);
			DrawResizeLine(hWnd, cp.y);
			oldy = cp.y;
		}
		break;
	case WM_LBUTTONUP:
		ReleaseCapture();
		bCapture = FALSE;
		SetCaretBlinkTime(u);
		GetCursorPos(&cp);
		hParent = GetParent(hWnd);
		GetClientRect(hParent, &rc);
		cx = rc.right - rc.left;
		cy = rc.bottom - rc.top;
		ScreenToClient(hParent, &cp);
		DrawResizeLine(hWnd, cp.y);
		y = cp.y;
		if(y < 80){
			y = 80;
		}
		else if(y > cy - 80){
			y = cy - 80;
		}
		MoveWindow(GetDlgItem(hParent, IDC_CHATAREA_EDIT),  4,         4, cx -  8,       y - 4, TRUE); 
		MoveWindow(GetDlgItem(hParent, IDC_STATIC_SPLIT),   4,         y, cx -  8,           8, TRUE); 
		MoveWindow(GetDlgItem(hParent, IDC_INPUTAREA_EDIT), 4,     y + 8, cx - 88, cy - y - 12, TRUE);
		break;
	}

	return CallWindowProc((WNDPROC)pDefSBProc, hWnd, uMsg, wParam, lParam);
}
