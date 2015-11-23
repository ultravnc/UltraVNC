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
#define ECONNRESET WSAECONNRESET
#define RFB_PORT_OFFSET 5900
#define true TRUE
#define false FALSE

typedef char rfbProtocolVersionMsg[13];



void debug( const char *fmt, ... );
void fatal( const char *fmt, ... );
BOOL ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *pport) ;
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

extern char temp1[50][16];
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