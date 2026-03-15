// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#ifndef TEXTCHAT_H__
#define TEXTCHAT_H__
#pragma once

#define BLACK	0x00000000
#define RED		0x000000FF
#define GREEN	0x0000FF00
#define BLUE	0x00FF0000
#define GREY	0x00888888

class ClientConnection;

class TextChat  
{
public:
	// Props
	VNCviewerApp		*m_pApp; 
	ClientConnection	*m_pCC;
	HWND				m_hWnd;
	HWND				m_hDlg;
	bool				m_fTextChatRunning;
	bool				m_fVisible;
	wchar_t*			m_szLocalText;
	wchar_t*			m_szLastLocalText;
	wchar_t*			m_szRemoteText;
	wchar_t*			m_szRemoteName;
	wchar_t*			m_szLocalName;
	wchar_t*			m_szTextBoxBuffer;
	bool				m_fPersistentTexts;
	HMODULE				m_hRichEdit;

	// Methods
	TextChat(VNCviewerApp *pApp, ClientConnection *pCC);
	int DoDialog();
	void KillDialog();
   	virtual ~TextChat();
	static BOOL CALLBACK TextChatDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SendTextChatRequest(int nMsg);
	void SendLocalText(void);
	void ProcessTextChatMsg();
	void PrintMessage(const wchar_t* szMessage, const wchar_t* szSender, DWORD color = BLACK);
	void SetTextFormat(bool bBold = false, bool bItalic = false, long nSize = 0x75, const wchar_t* szFaceName = L"MS Shell Dlg 2", DWORD dwColor = BLACK);
	void ShowChatWindow(bool fVisible);

};

#endif 


