/*
  Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
 Example header file

 Do not use this header file in the WiZ application, use WIZ.H
 instead.

*/
#ifndef _UZEXAMPL_H
#define _UZEXAMPL_H

#include <windows.h>
#ifdef __RSXNT__
#  include "../win32/rsxntwin.h"
#endif
#include <assert.h>    /* required for all Windows applications */
#include <stdlib.h>
#include <stdio.h>
#include <commdlg.h>
#ifndef __RSXNT__
#  include <dlgs.h>
#endif
#include <windowsx.h>

#include "../windll/structs.h"
#include "../windll/decs.h"

/* Defines */

typedef int (WINAPI * _DLL_UNZIP)(int, char **, int, char **,
                                  LPDCL, LPUSERFUNCTIONS);

/* Global variables */

extern LPUSERFUNCTIONS lpUserFunctions;
extern LPDCL lpDCL;

extern HINSTANCE hUnzipDll;

extern int hFile;                 /* file handle             */

/* Global functions */

int WINAPI DisplayBuf(LPSTR, unsigned long);

/* Procedure Calls */
void WINAPI ReceiveDllMessage(unsigned long, unsigned long, unsigned,
    unsigned, unsigned, unsigned, unsigned, unsigned,
    char, LPSTR, LPSTR, unsigned long, char);
#endif /* _UZEXAMPL_H */
