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


#include <locale.h>
#include <wchar.h>
#include <tchar.h>
#include "MSLogonACL.h"
#include "vncImportACL.h"
#include "vncExportACL.h"


int _tmain(int argc, TCHAR *argv[])
{
	setlocale( LC_ALL, "" );
	bool append = false;
	int rc = 1;

	if (argc > 1) {
		if (_tcsicmp(argv[1], _T("/i")) == 0 || _tcsicmp(argv[1], _T("-i")) == 0) {
			if (argc < 4) {
				return 1;
			} else {
				if (_tcsicmp(argv[2], _T("/a")) == 0 || _tcsicmp(argv[2], _T("-a")) == 0)
					append = true;
				else if (_tcsicmp(argv[2], _T("/o")) == 0 || _tcsicmp(argv[2], _T("-o")) == 0)
					; //override
				else {
					usage_(argv[0]);
					return 1;
				}
				if (!_tfreopen(argv[3], _T("r"), stdin)) {
					_tprintf(_T("Error opening file %s"), argv[3]);
					usage_(argv[0]);
					return 1;
				}
			}
			rc = import_(append);
		} else if (_tcsicmp(argv[1], _T("/e")) == 0 || _tcsicmp(argv[1], _T("-e")) == 0) {
			if (argc > 2)
				if (!_tfreopen(argv[2], _T("w"), stdout)) {
					_tprintf(_T("Error opening file %s"), argv[2]);
					usage_(argv[0]);
					return 1;
				}
			rc = export_();
		} else {
			usage_(argv[0]);
		}
	} else {
		usage_(argv[0]);
	}
	return rc;
}

int 
import_(bool append){
	int rc = 0;
	vncImportACL importAcl;
	PACL pACL = NULL; //?


		if (append)
			importAcl.GetOldACL();
		if (importAcl.ScanInput())
			rc |= 2;
		pACL = importAcl.BuildACL();
		importAcl.SetACL(pACL);

		HeapFree(GetProcessHeap(), 0, pACL);

	return rc;
}

int
export_()
{
	PACL pACL = NULL;

	vncExportACL exportAcl;
	exportAcl.GetACL(&pACL);
	exportAcl.PrintAcl(pACL);
	if (pACL)
		LocalFree(pACL);

	return 0;
}


void usage_(const TCHAR *appname){
	_tprintf(_T("Usage:\n%s /e <file>\n\t for exporting an ACL to an (optional) file.\n"), appname);
	_tprintf(_T("%s /i <mode> <file>\n\t for importing an ACL where mode is either\n"), appname);
	_tprintf(_T("\t/o for override or /a for append and file holds the ACEs.\n"));
	_tprintf(_T("For the format of the ACEs first configure some groups/users\n"));
	_tprintf(_T("with the graphical VNC Properties and then export the ACL.\n"));
	_tprintf(_T("The computer name can be replaced by a \".\" (a dot),\n"));
	_tprintf(_T("the computer's domain name by \"..\" (two dots).\n"));
}

