/*
  Copyright (c) 1990-2005 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2000-Apr-09 or later
  (the contents of which are also included in unzip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
#ifndef __unzip_structs_h
#define __unzip_structs_h

#ifndef Far
#  define Far far
#endif

/* Porting definitions between Win 3.1x and Win32 */
#ifdef WIN32
#  define far
#  define _far
#  define __far
#  define near
#  define _near
#  define __near
#  ifndef FAR
#    define FAR
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFINED_ONCE
#define DEFINED_ONCE

typedef int (WINAPI DLLPRNT) (LPSTR, unsigned long);
typedef int (WINAPI DLLPASSWORD) (LPSTR, int, LPCSTR, LPCSTR);
typedef int (WINAPI DLLSERVICE) (LPCSTR, unsigned long);
#endif /* DEFINED_ONCE */
typedef void (WINAPI DLLSND) (void);
typedef int (WINAPI DLLREPLACE)(LPSTR);
typedef void (WINAPI DLLMESSAGE)(unsigned long, unsigned long, unsigned,
   unsigned, unsigned, unsigned, unsigned, unsigned,
   char, LPSTR, LPSTR, unsigned long, char);

typedef struct {
DLLPRNT *print;
DLLSND *sound;
DLLREPLACE *replace;
DLLPASSWORD *password;
DLLMESSAGE *SendApplicationMessage;
DLLSERVICE *ServCallBk;
unsigned long TotalSizeComp;
unsigned long TotalSize;
unsigned long CompFactor;       /* "long" applied for proper alignment, only */
unsigned long NumMembers;
WORD cchComment;
} USERFUNCTIONS, far * LPUSERFUNCTIONS;

typedef struct {
int ExtractOnlyNewer;
int SpaceToUnderscore;
int PromptToOverwrite;
int fQuiet;
int ncflag;
int ntflag;
int nvflag;
int nfflag;
int nzflag;
int ndflag;
int noflag;
int naflag;
int nZIflag;
int C_flag;
int fPrivilege;
LPSTR lpszZipFN;
LPSTR lpszExtractDir;
} DCL, far * LPDCL;

#ifdef __cplusplus
}
#endif

/* return codes of the (DLLPASSWORD)() callback function */
#define IDM_REPLACE_NO     100
#define IDM_REPLACE_TEXT   101
#define IDM_REPLACE_YES    102
#define IDM_REPLACE_ALL    103
#define IDM_REPLACE_NONE   104
#define IDM_REPLACE_RENAME 105
#define IDM_REPLACE_HELP   106

#endif /* __unzip_structs_h */
