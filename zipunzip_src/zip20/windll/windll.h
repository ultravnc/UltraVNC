/*
  Copyright (c) 1990-1999 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 1999-Oct-05 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, both of these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.cdrom.com/pub/infozip/license.html
*/
/*
 WiZ 1.0 header file for zip dll
*/
#ifndef _WINDLL_H
#define _WINDLL_H

#include "structs.h"

#ifndef MSWIN
#define MSWIN
#endif

#ifndef USE_ZIPMAIN
#   define USE_ZIPMAIN
#endif

#ifndef NDEBUG
#  define WinAssert(exp) \
        {\
        if (!(exp))\
            {\
            char szBuffer[40];\
            sprintf(szBuffer, "File %s, Line %d",\
                    __FILE__, __LINE__) ;\
            if (IDABORT == MessageBox((HWND)NULL, szBuffer,\
                "Assertion Error",\
                MB_ABORTRETRYIGNORE|MB_ICONSTOP))\
                    FatalExit(-1);\
            }\
        }

#else
#  define WinAssert(exp)
#endif

#define cchFilesMax 4096

extern int WINAPI ZpArchive(ZCL C);
extern HWND hGetFilesDlg;
extern char szFilesToAdd[80];
extern char rgszFiles[cchFilesMax];
BOOL WINAPI CommentBoxProc(HWND hwndDlg, WORD wMessage, WPARAM wParam, LPARAM lParam);
BOOL PasswordProc(HWND, WORD, WPARAM, LPARAM);
void CenterDialog(HWND hwndParent, HWND hwndDlg);
void comment(unsigned int);

extern LPSTR szCommentBuf;
extern HANDLE hStr;
extern HWND hWndMain;
void __far __cdecl perror(const char *);

#endif /* _WINDLL_H */

