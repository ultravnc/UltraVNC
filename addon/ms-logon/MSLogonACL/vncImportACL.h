// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//


/*
 * vncImportACL.h: 
 */
#define _WIN32_WINNT	0x0500	//??
#define WINVER		0x0500	//??

#include <windows.h>
#include <aclapi.h>
#include <stdio.h>
#include <string.h>
#include <lm.h>

#define MAXLEN 256
#define lenof(a) (sizeof(a) / sizeof((a)[0]) )


#define ViewOnly	1
#define Interact	2
#define ALL_RIGHTS (ViewOnly | Interact)

class vncImportACL {
private:
	enum ACE_TYPE {
		ACCESS_ALLOWED,
			ACCESS_DENIED
	};
	
	typedef struct _ACE_DATA {
		ACE_TYPE type;
		DWORD mask;
		PSID pSID;
		struct _ACE_DATA *next;
	} ACE_DATA;

	ACE_DATA *lastAllowACE;
	ACE_DATA *lastDenyACE;

public:	
	vncImportACL();
	~vncImportACL();

	void GetOldACL();
	void ReadAce(int index, PACL pACL);
	PSID CopySID(PSID pSID);
	int ScanInput();
	bool FillAceData(const TCHAR *accesstype, 
		DWORD accessmask, 
		const TCHAR *domainaccount);
	PSID GetSID(const TCHAR *domainaccount);
	PACL BuildACL(void);
	bool SetACL(PACL pACL);
	const TCHAR * SplitString(const TCHAR *input, TCHAR separator, TCHAR *head);
	TCHAR *AddComputername(const TCHAR *user);
	TCHAR *AddDomainname(const TCHAR *user);
};

struct vncScanInput{
	bool isEmptyLine(TCHAR *line);
	void RemoveComment(TCHAR *line);
	void RemoveNewline(TCHAR *line);
	int GetQuoteLength(const TCHAR *line);
    size_t GetWordLength(const TCHAR *line);
	bool GetLine(TCHAR *line);
	bool Tokenize(const TCHAR *line, TCHAR **tokens);
	int AddToken(TCHAR **token, int tokenCount, const TCHAR **line, int len);
};
