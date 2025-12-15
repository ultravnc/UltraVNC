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
 *  vncExportACL.h
 */

#define _WIN32_WINNT	0x0500
#define WINVER			0x0500

#include <windows.h>
#include <aclapi.h>
#include <stdio.h>
#include <string.h>
#include <lm.h>

#define MAXLEN 256
#define lenof(a) (sizeof(a) / sizeof((a)[0]) )

class vncExportACL{
public:
	// returns the address of a !!static!!, non-thread-local, buffer with
	// the text representation of the SID that was passed in
	const TCHAR *SidToText( PSID psid );
	
	// Translates a SID and terminates it with a linefeed. No provision is
	// made to dump the SID in textual form if LookupAccountSid() fails.
	void PrintSid( PSID psid );
	
	// Displays the index-th (0-based) ACE from ACL
	void PrintAce(int index, PACL acl);
	
	// Dumps an entire ACL
	void PrintAcl(PACL acl);
	
	bool GetACL(PACL *pACL);
	
};