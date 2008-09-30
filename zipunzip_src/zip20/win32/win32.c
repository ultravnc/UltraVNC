/*
  Copyright (c) 1990-2006 Info-ZIP.  All rights reserved.

  See the accompanying file LICENSE, version 2005-Feb-10 or later
  (the contents of which are also included in zip.h) for terms of use.
  If, for some reason, all these files are missing, the Info-ZIP license
  also may be found at:  ftp://ftp.info-zip.org/pub/infozip/license.html
*/
/*
 * WIN32 specific functions for ZIP.
 *
 * The WIN32 version of ZIP heavily relies on the MSDOS and OS2 versions,
 * since we have to do similar things to switch between NTFS, HPFS and FAT.
 */


#include "../zip.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <windows.h>
#ifdef __RSXNT__
#  include <alloca.h>
#  include "../win32/rsxntwin.h"
#endif
#include "../win32/win32zip.h"

#define A_RONLY    0x01
#define A_HIDDEN   0x02
#define A_SYSTEM   0x04
#define A_LABEL    0x08
#define A_DIR      0x10
#define A_ARCHIVE  0x20


#define EAID     0x0009

#if (defined(__MINGW32__) && !defined(USE_MINGW_GLOBBING))
   int _CRT_glob = 0;   /* suppress command line globbing by C RTL */
#endif

#ifndef UTIL

extern int noisy;

#ifndef NO_W32TIMES_IZFIX
#if defined(NT_TZBUG_WORKAROUND)
local int FSusesLocalTime(const char *path);
#endif
#if defined(USE_EF_UT_TIME) || defined(NT_TZBUG_WORKAROUND)
local int NtfsFileTime2utime(const FILETIME *pft, time_t *ut);
#endif
#if defined(NT_TZBUG_WORKAROUND) && !defined(NO_W32TIMES_IZFIX)
local void utime2NtfsFileTime(time_t ut, FILETIME *pft);
#endif
#endif /* !NO_W32TIMES_IZFIX */

#if defined(NT_TZBUG_WORKAROUND) && defined(W32_STAT_BANDAID)
local int VFatFileTime2utime(const FILETIME *pft, time_t *ut);
#endif


/* FAT / HPFS detection */

int IsFileSystemOldFAT(const char *dir)
{
  static char lastDrive = '\0';    /* cached drive of last GetVolumeInformation call */
  static int lastDriveOldFAT = 0;  /* cached OldFAT value of last GetVolumeInformation call */
  char root[4];
  DWORD vfnsize;
  DWORD vfsflags;

    /*
     * We separate FAT and HPFS+other file systems here.
     * I consider other systems to be similar to HPFS/NTFS, i.e.
     * support for long file names and being case sensitive to some extent.
     */

    strncpy(root, dir, 3);
    if ( isalpha((uch)root[0]) && (root[1] == ':') ) {
      root[0] = to_up(dir[0]);
      root[2] = '\\';
      root[3] = 0;
    }
    else {
      root[0] = '\\';
      root[1] = 0;
    }
    if (lastDrive == root[0]) {
      return lastDriveOldFAT;
    }

    if ( !GetVolumeInformation(root, NULL, 0,
                               NULL, &vfnsize, &vfsflags,
                               NULL, 0)) {
        fprintf(mesg, "zip diagnostic: GetVolumeInformation failed\n");
        return(FALSE);
    }

    lastDrive = root[0];
    lastDriveOldFAT = vfnsize <= 12;

    return lastDriveOldFAT;
}


/* access mode bits and time stamp */

int GetFileMode(const char *name)
{
DWORD dwAttr;
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
  char *ansi_name = (char *)alloca(strlen(name) + 1);

  OemToAnsi(name, ansi_name);
  name = (const char *)ansi_name;
#endif

  dwAttr = GetFileAttributes(name);
  if ( dwAttr == 0xFFFFFFFF ) {
    fprintf(mesg, "zip diagnostic: GetFileAttributes failed\n");
    return(0x20); /* the most likely, though why the error? security? */
  }
  return(
          (dwAttr&FILE_ATTRIBUTE_READONLY  ? A_RONLY   :0)
        | (dwAttr&FILE_ATTRIBUTE_HIDDEN    ? A_HIDDEN  :0)
        | (dwAttr&FILE_ATTRIBUTE_SYSTEM    ? A_SYSTEM  :0)
        | (dwAttr&FILE_ATTRIBUTE_DIRECTORY ? A_DIR     :0)
        | (dwAttr&FILE_ATTRIBUTE_ARCHIVE   ? A_ARCHIVE :0));
}


#if defined(NT_TZBUG_WORKAROUND) && !defined(NO_W32TIMES_IZFIX)
local int FSusesLocalTime(const char *path)
{
    char  *tmp0;
    char   rootPathName[4];
    char   tmp1[MAX_PATH], tmp2[MAX_PATH];
    DWORD  volSerNo, maxCompLen, fileSysFlags;
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
    char *ansi_path = (char *)alloca(strlen(path) + 1);

    OemToAnsi(path, ansi_path);
    path = (const char *)ansi_path;
#endif

    if (isalpha((uch)path[0]) && (path[1] == ':'))
        tmp0 = (char *)path;
    else
    {
        GetFullPathName(path, MAX_PATH, tmp1, &tmp0);
        tmp0 = &tmp1[0];
    }
    strncpy(rootPathName, tmp0, 3);   /* Build the root path name, */
    rootPathName[3] = '\0';           /* e.g. "A:/"                */

    GetVolumeInformation((LPCTSTR)rootPathName, (LPTSTR)tmp1, (DWORD)MAX_PATH,
                         &volSerNo, &maxCompLen, &fileSysFlags,
                         (LPTSTR)tmp2, (DWORD)MAX_PATH);

    /* Volumes in (V)FAT and (OS/2) HPFS format store file timestamps in
     * local time!
     */
    return !strncmp(strupr(tmp2), "FAT", 3) ||
           !strncmp(tmp2, "VFAT", 4) ||
           !strncmp(tmp2, "HPFS", 4);

} /* end function FSusesLocalTime() */
#endif /* NT_TZBUG_WORKAROUND && !NO_W32TIMES_IZFIX */


#if defined(USE_EF_UT_TIME) || defined(NT_TZBUG_WORKAROUND)

#if (defined(__GNUC__) || defined(ULONG_LONG_MAX))
   typedef long long            LLONG64;
   typedef unsigned long long   ULLNG64;
#elif (defined(__WATCOMC__) && (__WATCOMC__ >= 1100))
   typedef __int64              LLONG64;
   typedef unsigned __int64     ULLNG64;
#elif (defined(_MSC_VER) && (_MSC_VER >= 1100))
   typedef __int64              LLONG64;
   typedef unsigned __int64     ULLNG64;
#elif (defined(__IBMC__) && (__IBMC__ >= 350))
   typedef __int64              LLONG64;
   typedef unsigned __int64     ULLNG64;
#else
#  define NO_INT64
#endif

/* scale factor and offset for conversion time_t -> FILETIME */
#  define NT_QUANTA_PER_UNIX 10000000L
#  define FTQUANTA_PER_UT_L  (NT_QUANTA_PER_UNIX & 0xFFFF)
#  define FTQUANTA_PER_UT_H  (NT_QUANTA_PER_UNIX >> 16)
#  define UNIX_TIME_ZERO_HI  0x019DB1DEUL
#  define UNIX_TIME_ZERO_LO  0xD53E8000UL
/* special FILETIME values for bound-checks */
#  define UNIX_TIME_UMAX_HI  0x0236485EUL
#  define UNIX_TIME_UMAX_LO  0xD4A5E980UL
#  define UNIX_TIME_SMIN_HI  0x0151669EUL
#  define UNIX_TIME_SMIN_LO  0xD53E8000UL
#  define UNIX_TIME_SMAX_HI  0x01E9FD1EUL
#  define UNIX_TIME_SMAX_LO  0xD4A5E980UL

#ifndef NO_W32TIMES_IZFIX
local int NtfsFileTime2utime(const FILETIME *pft, time_t *ut)
{
#ifndef NO_INT64
    ULLNG64 NTtime;

    NTtime = ((ULLNG64)pft->dwLowDateTime +
              ((ULLNG64)pft->dwHighDateTime << 32));

    /* underflow and overflow handling */
#ifdef CHECK_UTIME_SIGNED_UNSIGNED
    if ((time_t)0x80000000L < (time_t)0L)
    {
        if (NTtime < ((ULLNG64)UNIX_TIME_SMIN_LO +
                      ((ULLNG64)UNIX_TIME_SMIN_HI << 32))) {
            *ut = (time_t)LONG_MIN;
            return FALSE;
        }
        if (NTtime > ((ULLNG64)UNIX_TIME_SMAX_LO +
                      ((ULLNG64)UNIX_TIME_SMAX_HI << 32))) {
            *ut = (time_t)LONG_MAX;
            return FALSE;
        }
    }
    else
#endif /* CHECK_UTIME_SIGNED_UNSIGNED */
    {
        if (NTtime < ((ULLNG64)UNIX_TIME_ZERO_LO +
                      ((ULLNG64)UNIX_TIME_ZERO_HI << 32))) {
            *ut = (time_t)0;
            return FALSE;
        }
        if (NTtime > ((ULLNG64)UNIX_TIME_UMAX_LO +
                      ((ULLNG64)UNIX_TIME_UMAX_HI << 32))) {
            *ut = (time_t)ULONG_MAX;
            return FALSE;
        }
    }

    NTtime -= ((ULLNG64)UNIX_TIME_ZERO_LO +
               ((ULLNG64)UNIX_TIME_ZERO_HI << 32));
    *ut = (time_t)(NTtime / (unsigned long)NT_QUANTA_PER_UNIX);
    return TRUE;
#else /* NO_INT64 (64-bit integer arithmetics may not be supported) */
    /* nonzero if `y' is a leap year, else zero */
#   define leap(y) (((y)%4 == 0 && (y)%100 != 0) || (y)%400 == 0)
    /* number of leap years from 1970 to `y' (not including `y' itself) */
#   define nleap(y) (((y)-1969)/4 - ((y)-1901)/100 + ((y)-1601)/400)
    /* daycount at the end of month[m-1] */
    static ZCONST ush ydays[] =
      { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };

    time_t days;
    SYSTEMTIME w32tm;

    /* underflow and overflow handling */
#ifdef CHECK_UTIME_SIGNED_UNSIGNED
    if ((time_t)0x80000000L < (time_t)0L)
    {
        if ((pft->dwHighDateTime < UNIX_TIME_SMIN_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_SMIN_HI) &&
             (pft->dwLowDateTime < UNIX_TIME_SMIN_LO))) {
            *ut = (time_t)LONG_MIN;
            return FALSE;
        if ((pft->dwHighDateTime > UNIX_TIME_SMAX_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_SMAX_HI) &&
             (pft->dwLowDateTime > UNIX_TIME_SMAX_LO))) {
            *ut = (time_t)LONG_MAX;
            return FALSE;
        }
    }
    else
#endif /* CHECK_UTIME_SIGNED_UNSIGNED */
    {
        if ((pft->dwHighDateTime < UNIX_TIME_ZERO_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_ZERO_HI) &&
             (pft->dwLowDateTime < UNIX_TIME_ZERO_LO))) {
            *ut = (time_t)0;
            return FALSE;
        }
        if ((pft->dwHighDateTime > UNIX_TIME_UMAX_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_UMAX_HI) &&
             (pft->dwLowDateTime > UNIX_TIME_UMAX_LO))) {
            *ut = (time_t)ULONG_MAX;
            return FALSE;
        }
    }

    FileTimeToSystemTime(pft, &w32tm);

    /* set `days' to the number of days into the year */
    days = w32tm.wDay - 1 + ydays[w32tm.wMonth-1] +
           (w32tm.wMonth > 2 && leap (w32tm.wYear));

    /* now set `days' to the number of days since 1 Jan 1970 */
    days += 365 * (time_t)(w32tm.wYear - 1970) +
            (time_t)(nleap(w32tm.wYear));

    *ut = (time_t)(86400L * days + 3600L * (time_t)w32tm.wHour +
                   (time_t)(60 * w32tm.wMinute + w32tm.wSecond));
    return TRUE;
#endif /* ?NO_INT64 */
} /* end function NtfsFileTime2utime() */
#endif /* !NO_W32TIMES_IZFIX */
#endif /* USE_EF_UT_TIME || NT_TZBUG_WORKAROUND */


#if defined(NT_TZBUG_WORKAROUND) && defined(W32_STAT_BANDAID)

local int VFatFileTime2utime(const FILETIME *pft, time_t *ut)
{
    FILETIME lft;
    SYSTEMTIME w32tm;
    struct tm ltm;

    if (!FileTimeToLocalFileTime(pft, &lft)) {
        /* if pft cannot be converted to local time, return current time */
        return time(NULL);
    }
    FileTimeToSystemTime(&lft, &w32tm);
    /* underflow and overflow handling */
    /* TODO: The range checks are not accurate, the actual limits may
     *       be off by one daylight-saving-time shift (typically 1 hour),
     *       depending on the current state of "is_dst".
     */
#ifdef CHECK_UTIME_SIGNED_UNSIGNED
    if ((time_t)0x80000000L < (time_t)0L)
    {
        if ((pft->dwHighDateTime < UNIX_TIME_SMIN_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_SMIN_HI) &&
             (pft->dwLowDateTime < UNIX_TIME_SMIN_LO))) {
            *ut = (time_t)LONG_MIN;
            return FALSE;
        if ((pft->dwHighDateTime > UNIX_TIME_SMAX_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_SMAX_HI) &&
             (pft->dwLowDateTime > UNIX_TIME_SMAX_LO))) {
            *ut = (time_t)LONG_MAX;
            return FALSE;
        }
    }
    else
#endif /* CHECK_UTIME_SIGNED_UNSIGNED */
    {
        if ((pft->dwHighDateTime < UNIX_TIME_ZERO_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_ZERO_HI) &&
             (pft->dwLowDateTime < UNIX_TIME_ZERO_LO))) {
            *ut = (time_t)0;
            return FALSE;
        }
        if ((pft->dwHighDateTime > UNIX_TIME_UMAX_HI) ||
            ((pft->dwHighDateTime == UNIX_TIME_UMAX_HI) &&
             (pft->dwLowDateTime > UNIX_TIME_UMAX_LO))) {
            *ut = (time_t)ULONG_MAX;
            return FALSE;
        }
    }
    ltm.tm_year = w32tm.wYear - 1900;
    ltm.tm_mon = w32tm.wMonth - 1;
    ltm.tm_mday = w32tm.wDay;
    ltm.tm_hour = w32tm.wHour;
    ltm.tm_min = w32tm.wMinute;
    ltm.tm_sec = w32tm.wSecond;
    ltm.tm_isdst = -1;  /* let mktime determine if DST is in effect */
    *ut = mktime(&ltm);

    /* a cheap error check: mktime returns "(time_t)-1L" on conversion errors.
     * Normally, we would have to apply a consistency check because "-1"
     * could also be a valid time. But, it is quite unlikely to read back odd
     * time numbers from file systems that store time stamps in DOS format.
     * (The only known exception is creation time on VFAT partitions.)
     */
    return (*ut != (time_t)-1L);

} /* end function VFatFileTime2utime() */
#endif /* NT_TZBUG_WORKAROUND && W32_STAT_BANDAID */

#if defined(NT_TZBUG_WORKAROUND) && !defined(NO_W32TIMES_IZFIX)

local void utime2NtfsFileTime(time_t ut, FILETIME *pft)
{
#ifndef NO_INT64
    ULLNG64 NTtime;

    /* NT_QUANTA_PER_UNIX is small enough so that "ut * NT_QUANTA_PER_UNIX"
     * cannot overflow in 64-bit signed calculation, regardless whether "ut"
     * is signed or unsigned.  */
    NTtime = ((LLONG64)ut * NT_QUANTA_PER_UNIX) +
             ((ULLNG64)UNIX_TIME_ZERO_LO + ((ULLNG64)UNIX_TIME_ZERO_HI << 32));
    pft->dwLowDateTime = (DWORD)NTtime;
    pft->dwHighDateTime = (DWORD)(NTtime >> 32);

#else /* NO_INT64 (64-bit integer arithmetics may not be supported) */
    unsigned int b1, b2, carry = 0;
    unsigned long r0, r1, r2, r3;
    long r4;            /* signed, to catch environments with signed time_t */

    b1 = ut & 0xFFFF;
    b2 = (ut >> 16) & 0xFFFF;       /* if ut is over 32 bits, too bad */
    r1 = b1 * (NT_QUANTA_PER_UNIX & 0xFFFF);
    r2 = b1 * (NT_QUANTA_PER_UNIX >> 16);
    r3 = b2 * (NT_QUANTA_PER_UNIX & 0xFFFF);
    r4 = b2 * (NT_QUANTA_PER_UNIX >> 16);
    r0 = (r1 + (r2 << 16)) & 0xFFFFFFFFL;
    if (r0 < r1)
        carry++;
    r1 = r0;
    r0 = (r0 + (r3 << 16)) & 0xFFFFFFFFL;
    if (r0 < r1)
        carry++;
    pft->dwLowDateTime = r0 + UNIX_TIME_ZERO_LO;
    if (pft->dwLowDateTime < r0)
        carry++;
    pft->dwHighDateTime = r4 + (r2 >> 16) + (r3 >> 16)
                            + UNIX_TIME_ZERO_HI + carry;
#endif /* ?NO_INT64 */

} /* end function utime2NtfsFileTime() */
#endif /* NT_TZBUG_WORKAROUND && !NO_W32TIMES_IZFIX */


#if 0           /* Currently, this is not used at all */

long GetTheFileTime(const char *name, iztimes *z_ut)
{
  HANDLE h;
  FILETIME Modft, Accft, Creft, lft;
  WORD dh, dl;
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
  char *ansi_name = (char *)alloca(strlen(name) + 1);

  OemToAnsi(name, ansi_name);
  name = ansi_name;
#endif

  h = CreateFile(name, FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                 NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
  if ( h != INVALID_HANDLE_VALUE ) {
    BOOL ftOK = GetFileTime(h, &Creft, &Accft, &Modft);
    CloseHandle(h);
#ifdef USE_EF_UT_TIME
    if (ftOK && (z_ut != NULL)) {
      NtfsFileTime2utime(&Modft, &(z_ut->mtime));
      if (Accft.dwLowDateTime != 0 || Accft.dwHighDateTime != 0)
          NtfsFileTime2utime(&Accft, &(z_ut->atime));
      else
          z_ut->atime = z_ut->mtime;
      if (Creft.dwLowDateTime != 0 || Creft.dwHighDateTime != 0)
          NtfsFileTime2utime(&Creft, &(z_ut->ctime));
      else
          z_ut->ctime = z_ut->mtime;
    }
#endif
    FileTimeToLocalFileTime(&ft, &lft);
    FileTimeToDosDateTime(&lft, &dh, &dl);
    return(dh<<16) | dl;
  }
  else
    return 0L;
}

#endif /* never */


void ChangeNameForFAT(char *name)
{
  char *src, *dst, *next, *ptr, *dot, *start;
  static char invalid[] = ":;,=+\"[]<>| \t";

  if ( isalpha((uch)name[0]) && (name[1] == ':') )
    start = name + 2;
  else
    start = name;

  src = dst = start;
  if ( (*src == '/') || (*src == '\\') )
    src++, dst++;

  while ( *src )
  {
    for ( next = src; *next && (*next != '/') && (*next != '\\'); next++ );

    for ( ptr = src, dot = NULL; ptr < next; ptr++ )
      if ( *ptr == '.' )
      {
        dot = ptr; /* remember last dot */
        *ptr = '_';
      }

    if ( dot == NULL )
      for ( ptr = src; ptr < next; ptr++ )
        if ( *ptr == '_' )
          dot = ptr; /* remember last _ as if it were a dot */

    if ( dot && (dot > src) &&
         ((next - dot <= 4) ||
          ((next - src > 8) && (dot - src > 3))) )
    {
      if ( dot )
        *dot = '.';

      for ( ptr = src; (ptr < dot) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;

      for ( ptr = dot; (ptr < next) && ((ptr - dot) < 4); ptr++ )
        *dst++ = *ptr;
    }
    else
    {
      if ( dot && (next - src == 1) )
        *dot = '.';           /* special case: "." as a path component */

      for ( ptr = src; (ptr < next) && ((ptr - src) < 8); ptr++ )
        *dst++ = *ptr;
    }

    *dst++ = *next; /* either '/' or 0 */

    if ( *next )
    {
      src = next + 1;

      if ( *src == 0 ) /* handle trailing '/' on dirs ! */
        *dst = 0;
    }
    else
      break;
  }

  for ( src = start; *src != 0; ++src )
    if ( (strchr(invalid, *src) != NULL) || (*src == ' ') )
      *src = '_';
}

char *GetLongPathEA(const char *name)
{
    return(NULL); /* volunteers ? */
}

int IsFileNameValid(x)
const char *x;
{
    WIN32_FIND_DATA fd;
    HANDLE h;
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
    char *ansi_name = (char *)alloca(strlen(x) + 1);

    OemToAnsi(x, ansi_name);
    x = (const char *)ansi_name;
#endif

    if ((h = FindFirstFile(x, &fd)) == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(h);
    return TRUE;
}

char *getVolumeLabel(drive, vtime, vmode, vutim)
  int drive;    /* drive name: 'A' .. 'Z' or '\0' for current drive */
  ulg *vtime;   /* volume label creation time (DOS format) */
  ulg *vmode;   /* volume label file mode */
  time_t *vutim;/* volume label creationtime (UNIX format) */

/* If a volume label exists for the given drive, return its name and
   pretend to set its time and mode. The returned name is static data. */
{
  char rootpath[4];
  static char vol[14];
  DWORD fnlen, flags;

  *vmode = A_ARCHIVE | A_LABEL;           /* this is what msdos returns */
  *vtime = dostime(1980, 1, 1, 0, 0, 0);  /* no true date info available */
  *vutim = dos2unixtime(*vtime);
  strcpy(rootpath, "x:\\");
  rootpath[0] = (char)drive;
  if (GetVolumeInformation(drive ? rootpath : NULL, vol, 13, NULL,
                           &fnlen, &flags, NULL, 0))
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
    return (AnsiToOem(vol, vol), vol);
#else
    return vol;
#endif
  else
    return NULL;
}



void stamp(f, d)
char *f;                /* name of file to change */
ulg d;                  /* dos-style time to change it to */
/* Set last updated and accessed time of file f to the DOS time d. */
{
#ifdef NT_TZBUG_WORKAROUND

    FILETIME Modft;     /* File time type of Win32 API, `last modified' time */
    HANDLE hFile;       /* File handle defined in Win32 API    */
# ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
    char *ansi_name = (char *)alloca(strlen(f) + 1);

    OemToAnsi(f, ansi_name);
#   define Ansi_Fname  ansi_name
# else
#   define Ansi_Fname  f
# endif

    /* open a handle to the file to prepare setting the mod-time stamp */
    hFile = CreateFile(Ansi_Fname, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if ( hFile != INVALID_HANDLE_VALUE ) {
        /* convert time_t modtime into WIN32 native 64bit format */
# ifndef NO_W32TIMES_IZFIX
        if (!FSusesLocalTime(f)) {
            time_t ux_modtime = dos2unixtime(d);
            utime2NtfsFileTime(ux_modtime, &Modft);
        } else
# endif /* !NO_W32TIMES_IZFIX */
        {
            FILETIME lft;

            DosDateTimeToFileTime((WORD)(d >> 16),
                                  (WORD)(d & 0xFFFFL),
                                  &lft);
            LocalFileTimeToFileTime(&lft, &Modft);
        }

        /* set Access and Modification times of the file to modtime */
        SetFileTime(hFile, NULL, &Modft, &Modft);
        CloseHandle(hFile);
    }

#else /* !NT_TZBUG_WORKAROUND */

    struct utimbuf u;   /* argument for utime() */

    /* Convert DOS time to time_t format in u.actime and u.modtime */
    u.actime = u.modtime = dos2unixtime(d);

    /* Set updated and accessed times of f */
    utime(f, &u);

#endif /* ?NT_TZBUG_WORKAROUND */
}

#endif /* !UTIL */



int ZipIsWinNT(void)    /* returns TRUE if real NT, FALSE if Win95 or Win32s */
{
    static DWORD g_PlatformId = 0xFFFFFFFF; /* saved platform indicator */

    if (g_PlatformId == 0xFFFFFFFF) {
        /* note: GetVersionEx() doesn't exist on WinNT 3.1 */
        if (GetVersion() < 0x80000000)
            g_PlatformId = TRUE;
        else
            g_PlatformId = FALSE;
    }
    return (int)g_PlatformId;
}


#ifndef UTIL
#ifdef __WATCOMC__
#  include <io.h>
#  define _get_osfhandle _os_handle
/* gaah -- Watcom's docs claim that _get_osfhandle exists, but it doesn't.  */
#endif

#ifdef HAVE_FSEEKABLE
/*
 * return TRUE if file is seekable
 */
int fseekable(fp)
FILE *fp;
{
    return GetFileType((HANDLE)_get_osfhandle(fileno(fp))) == FILE_TYPE_DISK;
}
#endif /* HAVE_FSEEKABLE */
#endif /* !UTIL */


#if 0 /* seems to be never used; try it out... */
char *StringLower(char *szArg)
{
  char *szPtr;
/*  unsigned char *szPtr; */
  for ( szPtr = szArg; *szPtr; szPtr++ )
    *szPtr = lower[*szPtr];
  return szArg;
}
#endif /* never */



#ifdef W32_STAT_BANDAID

/* All currently known variants of WIN32 operating systems (Windows 95/98,
 * WinNT 3.x, 4.0, 5.x) have a nasty bug in the OS kernel concerning
 * conversions between UTC and local time: In the time conversion functions
 * of the Win32 API, the timezone offset (including seasonal daylight saving
 * shift) between UTC and local time evaluation is erratically based on the
 * current system time. The correct evaluation must determine the offset
 * value as it {was/is/will be} for the actual time to be converted.
 *
 * Newer versions of MS C runtime lib's stat() returns utc time-stamps so
 * that localtime(timestamp) corresponds to the (potentially false) local
 * time shown by the OS' system programs (Explorer, command shell dir, etc.)
 * The RSXNT port follows the same strategy, but fails to recognize the
 * access-time attribute.
 *
 * For the NTFS file system (and other filesystems that store time-stamps
 * as UTC values), this results in st_mtime (, st_{c|a}time) fields which
 * are not stable but vary according to the seasonal change of "daylight
 * saving time in effect / not in effect".
 *
 * Other C runtime libs (CygWin, or the crtdll.dll supplied with Win9x/NT
 * return the unix-time equivalent of the UTC FILETIME values as got back
 * from the Win32 API call. This time, return values from NTFS are correct
 * whereas utimes from files on (V)FAT volumes vary according to the DST
 * switches.
 *
 * To achieve timestamp consistency of UTC (UT extra field) values in
 * Zip archives, the Info-ZIP programs require work-around code for
 * proper time handling in stat() (and other time handling routines).
 *
 * However, nowadays most other programs on Windows systems use the
 * time conversion strategy of Microsofts C runtime lib "msvcrt.dll".
 * To improve interoperability in environments where a "consistent" (but
 * false) "UTC<-->LocalTime" conversion is preferred over "stable" time
 * stamps, the Info-ZIP specific time conversion handling can be
 * deactivated by defining the preprocessor flag NO_W32TIMES_IZFIX.
 */
/* stat() functions under Windows95 tend to fail for root directories.   *
 * Watcom and Borland, at least, are affected by this bug.  Watcom made  *
 * a partial fix for 11.0 but still missed some cases.  This substitute  *
 * detects the case and fills in reasonable values.  Otherwise we get    *
 * effects like failure to extract to a root dir because it's not found. */

int zstat_zipwin32(const char *path, struct stat *buf)
{
    if (!stat(path, buf))
    {
#if (!defined(UTIL) && defined(NT_TZBUG_WORKAROUND))
        /* stat was successful, now redo the time-stamp fetches */
#ifndef NO_W32TIMES_IZFIX
        int fs_uses_loctime = FSusesLocalTime(path);
#endif
        HANDLE h;
        FILETIME Modft, Accft, Creft;
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
        char *ansi_path = (char *)alloca(strlen(path) + 1);

        OemToAnsi(path, ansi_path);
#       define Ansi_Path  ansi_path
#else
#       define Ansi_Path  path
#endif

        Trace((stdout, "stat(%s) finds modtime %08lx\n", path, buf->st_mtime));
        h = CreateFile(Ansi_Path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            BOOL ftOK = GetFileTime(h, &Creft, &Accft, &Modft);
            CloseHandle(h);

            if (ftOK) {
#ifndef NO_W32TIMES_IZFIX
                if (!fs_uses_loctime) {
                    /*  On a filesystem that stores UTC timestamps, we refill
                     *  the time fields of the struct stat buffer by directly
                     *  using the UTC values as returned by the Win32
                     *  GetFileTime() API call.
                     */
                    NtfsFileTime2utime(&Modft, &(buf->st_mtime));
                    if (Accft.dwLowDateTime != 0 || Accft.dwHighDateTime != 0)
                        NtfsFileTime2utime(&Accft, &(buf->st_atime));
                    else
                        buf->st_atime = buf->st_mtime;
                    if (Creft.dwLowDateTime != 0 || Creft.dwHighDateTime != 0)
                        NtfsFileTime2utime(&Creft, &(buf->st_ctime));
                    else
                        buf->st_ctime = buf->st_mtime;
                    Tracev((stdout,"NTFS, recalculated modtime %08lx\n",
                            buf->st_mtime));
                } else
#endif /* NO_W32TIMES_IZFIX */
                {
                    /*  On VFAT and FAT-like filesystems, the FILETIME values
                     *  are converted back to the stable local time before
                     *  converting them to UTC unix time-stamps.
                     */
                    VFatFileTime2utime(&Modft, &(buf->st_mtime));
                    if (Accft.dwLowDateTime != 0 || Accft.dwHighDateTime != 0)
                        VFatFileTime2utime(&Accft, &(buf->st_atime));
                    else
                        buf->st_atime = buf->st_mtime;
                    if (Creft.dwLowDateTime != 0 || Creft.dwHighDateTime != 0)
                        VFatFileTime2utime(&Creft, &(buf->st_ctime));
                    else
                        buf->st_ctime = buf->st_mtime;
                    Tracev((stdout, "VFAT, recalculated modtime %08lx\n",
                            buf->st_mtime));
                }
            }
        }
#       undef Ansi_Path
#endif /* !UTIL && NT_TZBUG_WORKAROUND */
        return 0;
    }
#ifdef W32_STATROOT_FIX
    else
    {
        DWORD flags;
#ifdef __RSXNT__        /* RSXNT/EMX C rtl uses OEM charset */
        char *ansi_path = (char *)alloca(strlen(path) + 1);

        OemToAnsi(path, ansi_path);
#       define Ansi_Path  ansi_path
#else
#       define Ansi_Path  path
#endif

        flags = GetFileAttributes(Ansi_Path);
        if (flags != 0xFFFFFFFF && flags & FILE_ATTRIBUTE_DIRECTORY) {
            Trace((stderr, "\nstat(\"%s\",...) failed on existing directory\n",
                   path));
            memset(buf, 0, sizeof(struct stat));
            buf->st_atime = buf->st_ctime = buf->st_mtime =
              dos2unixtime(DOSTIME_MINIMUM);
            /* !!!   you MUST NOT add a cast to the type of "st_mode" here;
             * !!!   different compilers do not agree on the "st_mode" size!
             * !!!   (And, some compiler may not declare the "mode_t" type
             * !!!   identifier, so you cannot use it, either.)
             */
            buf->st_mode = S_IFDIR | S_IREAD |
                           ((flags & FILE_ATTRIBUTE_READONLY) ? 0 : S_IWRITE);
            return 0;
        } /* assumes: stat() won't fail on non-dirs without good reason */
#       undef Ansi_Path
    }
#endif /* W32_STATROOT_FIX */
    return -1;
}

#endif /* W32_STAT_BANDAID */



#ifdef W32_USE_IZ_TIMEZONE
#include "timezone.h"
#define SECSPERMIN      60
#define MINSPERHOUR     60
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
static void conv_to_rule(LPSYSTEMTIME lpw32tm, struct rule * ZCONST ptrule);

static void conv_to_rule(LPSYSTEMTIME lpw32tm, struct rule * ZCONST ptrule)
{
    if (lpw32tm->wYear != 0) {
        ptrule->r_type = JULIAN_DAY;
        ptrule->r_day = ydays[lpw32tm->wMonth - 1] + lpw32tm->wDay;
    } else {
        ptrule->r_type = MONTH_NTH_DAY_OF_WEEK;
        ptrule->r_mon = lpw32tm->wMonth;
        ptrule->r_day = lpw32tm->wDayOfWeek;
        ptrule->r_week = lpw32tm->wDay;
    }
    ptrule->r_time = (long)lpw32tm->wHour * SECSPERHOUR +
                     (long)(lpw32tm->wMinute * SECSPERMIN) +
                     (long)lpw32tm->wSecond;
}

int GetPlatformLocalTimezone(register struct state * ZCONST sp,
        void (*fill_tzstate_from_rules)(struct state * ZCONST sp_res,
                                        ZCONST struct rule * ZCONST start,
                                        ZCONST struct rule * ZCONST end))
{
    TIME_ZONE_INFORMATION tzinfo;
    DWORD res;

    /* read current timezone settings from registry if TZ envvar missing */
    res = GetTimeZoneInformation(&tzinfo);
    if (res != TIME_ZONE_ID_INVALID)
    {
        struct rule startrule, stoprule;

        conv_to_rule(&(tzinfo.StandardDate), &stoprule);
        conv_to_rule(&(tzinfo.DaylightDate), &startrule);
        sp->timecnt = 0;
        sp->ttis[0].tt_abbrind = 0;
        if ((sp->charcnt =
             WideCharToMultiByte(CP_ACP, 0, tzinfo.StandardName, -1,
                                 sp->chars, sizeof(sp->chars), NULL, NULL))
            == 0)
            sp->chars[sp->charcnt++] = '\0';
        sp->ttis[1].tt_abbrind = sp->charcnt;
        sp->charcnt +=
            WideCharToMultiByte(CP_ACP, 0, tzinfo.DaylightName, -1,
                                sp->chars + sp->charcnt,
                                sizeof(sp->chars) - sp->charcnt, NULL, NULL);
        if ((sp->charcnt - sp->ttis[1].tt_abbrind) == 0)
            sp->chars[sp->charcnt++] = '\0';
        sp->ttis[0].tt_gmtoff = - (tzinfo.Bias + tzinfo.StandardBias)
                                * MINSPERHOUR;
        sp->ttis[1].tt_gmtoff = - (tzinfo.Bias + tzinfo.DaylightBias)
                                * MINSPERHOUR;
        sp->ttis[0].tt_isdst = 0;
        sp->ttis[1].tt_isdst = 1;
        sp->typecnt = (startrule.r_mon == 0 && stoprule.r_mon == 0) ? 1 : 2;

        if (sp->typecnt > 1)
            (*fill_tzstate_from_rules)(sp, &startrule, &stoprule);
        return TRUE;
    }
    return FALSE;
}
#endif /* W32_USE_IZ_TIMEZONE */



#ifndef WINDLL
/* This replacement getch() function was originally created for Watcom C
 * and then additionally used with CYGWIN. Since UnZip 5.4, all other Win32
 * ports apply this replacement rather that their supplied getch() (or
 * alike) function.  There are problems with unabsorbed LF characters left
 * over in the keyboard buffer under Win95 (and 98) when ENTER was pressed.
 * (Under Win95, ENTER returns two(!!) characters: CR-LF.)  This problem
 * does not appear when run on a WinNT console prompt!
 */

/* Watcom 10.6's getch() does not handle Alt+<digit><digit><digit>. */
/* Note that if PASSWD_FROM_STDIN is defined, the file containing   */
/* the password must have a carriage return after the word, not a   */
/* Unix-style newline (linefeed only).  This discards linefeeds.    */

int getch_win32(void)
{
  HANDLE stin;
  DWORD rc;
  unsigned char buf[2];
  int ret = -1;
  DWORD odemode = ~(DWORD)0;

#  ifdef PASSWD_FROM_STDIN
  stin = GetStdHandle(STD_INPUT_HANDLE);
#  else
  stin = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (stin == INVALID_HANDLE_VALUE)
    return -1;
#  endif
  if (GetConsoleMode(stin, &odemode))
    SetConsoleMode(stin, ENABLE_PROCESSED_INPUT);  /* raw except ^C noticed */
  if (ReadFile(stin, &buf, 1, &rc, NULL) && rc == 1)
    ret = buf[0];
  /* when the user hits return we get CR LF.  We discard the LF, not the CR,
   * because when we call this for the first time after a previous input
   * such as the one for "replace foo? [y]es, ..." the LF may still be in
   * the input stream before whatever the user types at our prompt. */
  if (ret == '\n')
    if (ReadFile(stin, &buf, 1, &rc, NULL) && rc == 1)
      ret = buf[0];
  if (odemode != ~(DWORD)0)
    SetConsoleMode(stin, odemode);
#  ifndef PASSWD_FROM_STDIN
  CloseHandle(stin);
#  endif
  return ret;
}



/******************************/
/*  Function version_local()  */
/******************************/

void version_local()
{
    static ZCONST char CompiledWith[] = "Compiled with %s%s for %s%s%s.\n\n";
#if (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__DJGPP__))
    char buf[80];
#if (defined(_MSC_VER) && (_MSC_VER > 900))
    char buf2[80];
#endif
#endif

/* Define the compiler name and version strings */
#if defined(_MSC_VER)  /* MSC == MSVC++, including the SDK compiler */
    sprintf(buf, "Microsoft C %d.%02d ", _MSC_VER/100, _MSC_VER%100);
#  define COMPILER_NAME1        buf
#  if (_MSC_VER == 800)
#    define COMPILER_NAME2      "(Visual C++ v1.1)"
#  elif (_MSC_VER == 850)
#    define COMPILER_NAME2      "(Windows NT v3.5 SDK)"
#  elif (_MSC_VER == 900)
#    define COMPILER_NAME2      "(Visual C++ v2.x)"
#  elif (_MSC_VER > 900)
    sprintf(buf2, "(Visual C++ v%d.%d)", _MSC_VER/100 - 6, _MSC_VER%100/10);
#    define COMPILER_NAME2      buf2
#  else
#    define COMPILER_NAME2      "(bad version)"
#  endif
#elif defined(__WATCOMC__)
#  if (__WATCOMC__ % 10 > 0)
/* We do this silly test because __WATCOMC__ gives two digits for the  */
/* minor version, but Watcom packaging prefers to show only one digit. */
    sprintf(buf, "Watcom C/C++ %d.%02d", __WATCOMC__ / 100,
            __WATCOMC__ % 100);
#  else
    sprintf(buf, "Watcom C/C++ %d.%d", __WATCOMC__ / 100,
            (__WATCOMC__ % 100) / 10);
#  endif /* __WATCOMC__ % 10 > 0 */
#  define COMPILER_NAME1        buf
#  define COMPILER_NAME2        ""
#elif defined(__TURBOC__)
#  ifdef __BORLANDC__
#    define COMPILER_NAME1      "Borland C++"
#    if (__BORLANDC__ == 0x0452)   /* __BCPLUSPLUS__ = 0x0320 */
#      define COMPILER_NAME2    " 4.0 or 4.02"
#    elif (__BORLANDC__ == 0x0460)   /* __BCPLUSPLUS__ = 0x0340 */
#      define COMPILER_NAME2    " 4.5"
#    elif (__BORLANDC__ == 0x0500)   /* __TURBOC__ = 0x0500 */
#      define COMPILER_NAME2    " 5.0"
#    elif (__BORLANDC__ == 0x0520)   /* __TURBOC__ = 0x0520 */
#      define COMPILER_NAME2    " 5.2 (C++ Builder 1.0)"
#    elif (__BORLANDC__ == 0x0530)   /* __BCPLUSPLUS__ = 0x0530 */
#      define COMPILER_NAME2    " 5.3 (C++ Builder 3.0)"
#    elif (__BORLANDC__ == 0x0540)   /* __BCPLUSPLUS__ = 0x0540 */
#      define COMPILER_NAME2    " 5.4 (C++ Builder 4.0)"
#    elif (__BORLANDC__ == 0x0550)   /* __BCPLUSPLUS__ = 0x0550 */
#      define COMPILER_NAME2    " 5.5 (C++ Builder 5.0)"
#    elif (__BORLANDC__ == 0x0551)   /* __BCPLUSPLUS__ = 0x0551 */
#      define COMPILER_NAME2    " 5.5.1 (C++ Builder 5.0.1)"
#    elif (__BORLANDC__ == 0x0560)   /* __BCPLUSPLUS__ = 0x0560 */
#      define COMPILER_NAME2    " 5.6 (C++ Builder 6)"
#    else
#      define COMPILER_NAME2    " later than 5.6"
#    endif
#  else /* !__BORLANDC__ */
#    define COMPILER_NAME1      "Turbo C"
#    if (__TURBOC__ >= 0x0400)     /* Kevin:  3.0 -> 0x0401 */
#      define COMPILER_NAME2    "++ 3.0 or later"
#    elif (__TURBOC__ == 0x0295)     /* [661] vfy'd by Kevin */
#      define COMPILER_NAME2    "++ 1.0"
#    endif
#  endif /* __BORLANDC__ */
#elif defined(__GNUC__)
#  ifdef __RSXNT__
#    if (defined(__DJGPP__) && !defined(__EMX__))
    sprintf(buf, "rsxnt(djgpp v%d.%02d) / gcc ",
            __DJGPP__, __DJGPP_MINOR__);
#      define COMPILER_NAME1    buf
#    elif defined(__DJGPP__)
    sprintf(buf, "rsxnt(emx+djgpp v%d.%02d) / gcc ",
            __DJGPP__, __DJGPP_MINOR__);
#      define COMPILER_NAME1    buf
#    elif (defined(__GO32__) && !defined(__EMX__))
#      define COMPILER_NAME1    "rsxnt(djgpp v1.x) / gcc "
#    elif defined(__GO32__)
#      define COMPILER_NAME1    "rsxnt(emx + djgpp v1.x) / gcc "
#    elif defined(__EMX__)
#      define COMPILER_NAME1    "rsxnt(emx)+gcc "
#    else
#      define COMPILER_NAME1    "rsxnt(unknown) / gcc "
#    endif
#  elif defined(__CYGWIN__)
#      define COMPILER_NAME1    "Cygnus win32 / gcc "
#  elif defined(__MINGW32__)
#      define COMPILER_NAME1    "mingw32 / gcc "
#  else
#      define COMPILER_NAME1    "gcc "
#  endif
#  define COMPILER_NAME2        __VERSION__
#elif defined(__LCC__)
#  define COMPILER_NAME1        "LCC-Win32"
#  define COMPILER_NAME2        ""
#else
#  define COMPILER_NAME1        "unknown compiler (SDK?)"
#  define COMPILER_NAME2        ""
#endif

/* Define the compile date string */
#ifdef __DATE__
#  define COMPILE_DATE " on " __DATE__
#else
#  define COMPILE_DATE ""
#endif

    printff(CompiledWith, COMPILER_NAME1, COMPILER_NAME2,
           "\nWindows 9x / Windows NT/2000/XP/etc.", " (32-bit)", COMPILE_DATE);

    return;

} /* end function version_local() */
#endif /* !WINDLL */
