/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>

#include <windows.h>
#include <winsock.h>
#include <sys/stat.h>
#include <io.h>
#include <conio.h>
#include <tchar.h>

#ifdef __cplusplus
 extern "C" {
 #endif 
#include "./webgui/webgui.h"
 #ifdef __cplusplus
 }
 #endif
#include "list_functions.h"

#define socket_errno() WSAGetLastError()
#define rfbProtocolVersionFormat "RFB %03d.%03d\n"
 #define rfbProtocolKeepAlive "REP %03d.%03d\n"
#define rfbProtocolMajorVersion 0
#define rfbProtocolMinorVersion 0
#define sz_rfbProtocolVersionMsg 12
#define MAX_HOST_NAME_LEN 250
#define MAX_IP 1000

#define LOCAL_SOCKET	1
#define METHOD_DIRECT    1
#ifndef FD_ALLOC
#define FD_ALLOC(nfds) ((fd_set*)malloc((nfds+7)/8))
#endif
#ifndef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif
#define RFB_PORT_OFFSET 5900
#define true TRUE
#define false FALSE

typedef char rfbProtocolVersionMsg[13];



void debug( const char *fmt, ... );
void fatal( const char *fmt, ... );
BOOL ParseDisplay(LPTSTR display, int size, LPTSTR phost, int hostlen, int *pport) ;
int WriteExact(int sock, char *buf, int len);
int ReadExact(int sock, char *buf, int len);

DWORD WINAPI do_repeater_wait(LPVOID lpParam);
DWORD WINAPI do_repeater(LPVOID lpParam);

void Start_server_listenThread();
void Start_mode12listenerThread();
void Stop_server_listenThread();
void Stop_mode12listenerThread();

extern int notstopped;


//Settings

extern char temp1[50][25];
extern char temp2[50][16];
extern char temp3[50][16];
extern int rule1;
extern int rule2;
extern int rule3;



void LogStats(int code,long recv,long send);
void LogStats_viewer(int code,int nummer);
void LogStats_server(int code,int nummer);
void LogStats_access(char *start,char *stop,int code,int viewer,int server ,long bytes);
void error( const char *fmt, ... );
void fatal( const char *fmt, ... );
void report_bytes( char *prefix, char *buf, int len );