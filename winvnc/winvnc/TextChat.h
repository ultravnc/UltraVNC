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

class vncClient;

class TextChat  
{
public:
	// Props
	vncClient			*m_pCC;
	HWND				m_hWnd;
	HWND				m_hDlg;
	bool				m_fTextChatRunning;
	char*				m_szLocalText;
	char*				m_szLastLocalText;
	char*				m_szRemoteText;	
	char*				m_szRemoteName;
	char*				m_szLocalName;
	char*				m_szTextBoxBuffer;
	HMODULE				m_hRichEdit;
	bool				m_fPersistentTexts;
	// char				m_szUserID[16];
    HANDLE              m_Thread;

	// Methods
	TextChat(vncClient *pCC);
//	int DoDialog();
	HWND DisplayTextChat();
	void KillDialog();
	void OrderTextChat();
	void RefuseTextChat();
	void ProcessTextChatMsg(int nTO);
   	virtual ~TextChat();
	static LRESULT CALLBACK DoDialogThread(LPVOID lpParameter);
	static INT_PTR CALLBACK TextChatDlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SendTextChatRequest(int Msg);
	void SendLocalText(void);
	void PrintMessage(const char* szMessage, const char* szSender, DWORD color = BLACK);
	void SetTextFormat(bool bBold = false, bool bItalic = false, long nSize = 0x75, const char* szFaceName = "MS Sans Serif", DWORD dwColor = BLACK);
	void ShowChatWindow(bool fVisible);
};

#endif 



