/*
  Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
#ifndef __decs_h   /* prevent multiple inclusions */
#define __decs_h

#include <windows.h>
#ifndef __unzip_h
#  include "../unzip.h"
#endif
#ifndef __structs_h
#  include "../windll/structs.h"
#endif

#define Wiz_Match match

#ifdef __cplusplus
extern "C" {
#endif

void    WINAPI Wiz_NoPrinting(int f);
int     WINAPI Wiz_Validate(LPSTR archive, int AllCodes);
BOOL    WINAPI Wiz_Init(zvoid *, LPUSERFUNCTIONS);
BOOL    WINAPI Wiz_SetOpts(zvoid *, LPDCL);
int     WINAPI Wiz_Unzip(zvoid *, int, char **, int, char **);
int     WINAPI Wiz_SingleEntryUnzip(int, char **, int, char **,
                                    LPDCL, LPUSERFUNCTIONS);

int     WINAPI Wiz_UnzipToMemory(LPSTR zip, LPSTR file,
                                 LPUSERFUNCTIONS lpUserFunctions,
                                 UzpBuffer *retstr);
int     WINAPI Wiz_Grep(LPSTR archive, LPSTR file, LPSTR pattern,
                        int cmd, int SkipBin,
                        LPUSERFUNCTIONS lpUserFunctions);

#ifdef __cplusplus
}
#endif

#endif /* __decs_h */
