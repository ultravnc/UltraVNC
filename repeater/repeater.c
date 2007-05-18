/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
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
//	Program is based on the
//  http://www.imasy.or.jp/~gotoh/ssh/connect.c
//  Written By Shun-ichi GOTO <gotoh@taiyo.co.jp>
//
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://ultravnc.sourceforge.net/
//
/////////////////////////////////////////////////////////////////////////////


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

#define LOCAL_SOCKET	1
#define METHOD_DIRECT    1
#define socket_errno() WSAGetLastError()
#ifndef FD_ALLOC
#define FD_ALLOC(nfds) ((fd_set*)malloc((nfds+7)/8))
#endif
#define ECONNRESET WSAECONNRESET

#define rfbProtocolVersionFormat "RFB %03d.%03d\n"
#define rfbProtocolMajorVersion 0
#define rfbProtocolMinorVersion 0
#define sz_rfbProtocolVersionMsg 12
#define MAX_HOST_NAME_LEN 250
#define RFB_PORT_OFFSET 5900
typedef char rfbProtocolVersionMsg[13];	/* allow extra byte for null */
#define true TRUE
#define false FALSE


typedef struct _mystruct
{
	SOCKET local_in;
	SOCKET local_out;
	SOCKET remote;
	ULONG	code;
	DWORD timestamp;
	BOOL used;
}mystruct,*pmystruct;

mystruct Servers[20];
mystruct Viewers[20];

int   local_type ;
int   relay_method;
int f_debug;
BOOL notstopped;

u_short local_port;
u_short server_port;

char *dest_host = NULL;
u_short dest_port;

int ReadExact(int sock, char *buf, int len);
ULONG  Find_viewer_list(mystruct *Viewerstruct);

void
debug( const char *fmt, ... )
{
    va_list args;
    if ( f_debug ) {
	va_start( args, fmt );
	fprintf(stderr, "UltraVnc> ");
	vfprintf( stderr, fmt, args );
	va_end( args );
    }
}

void
Clean_server_List()
{
	int i;
	for (i=0;i<20;i++)
	{
		Servers[i].code=0;
		Servers[i].used=false;
	}
}

void
Add_server_list(mystruct *Serverstruct)
{
	int i;
	for (i=0;i<20;i++)
	{
		if (Servers[i].code==Serverstruct->code) return;
	}
	for (i=0;i<20;i++)
	{
		if (Servers[i].code==0) 
		{
			debug("Server added to list %i\n",Serverstruct->code);
			Servers[i].code=Serverstruct->code;
			Servers[i].local_in=Serverstruct->local_in;
			Servers[i].local_out=Serverstruct->local_out;
			Servers[i].remote=Serverstruct->remote;
			Servers[i].timestamp=GetTickCount();
			Servers[i].used=false;
			return;
		}
	}

}

void 
Remove_server_list(ULONG code)
{
	int i;
	for (i=0;i<20;i++)
	{
		if (Servers[i].code==code)
		{
			debug("Server Removed from list %i\n",code);
			Servers[i].code=0;
			Servers[i].used=false;
			return;
		}
	}
}

ULONG 
Find_server_list(mystruct *Serverstruct)
{
	int i;
	for (i=0;i<20;i++)
	{
		if (Servers[i].code==Serverstruct->code)
		{
			Servers[i].used=true;
			return i;
		}
	}
	return 99;
}


void
Clean_viewer_List()
{
	int i;
	for (i=0;i<20;i++)
	{
		Viewers[i].code=0;
		Servers[i].used=false;
	}
}

void
Add_viewer_list(mystruct *Viewerstruct)
{
	int i;
	for (i=0;i<20;i++)
	{
		if (Viewers[i].code==Viewerstruct->code) return;
	}
	for (i=0;i<20;i++)
	{
		if (Viewers[i].code==0) 
		{
			debug("Viewer added to list %i\n",Viewerstruct->code);
			Viewers[i].code=Viewerstruct->code;
			Viewers[i].local_in=Viewerstruct->local_in;
			Viewers[i].local_out=Viewerstruct->local_out;
			Viewers[i].remote=Viewerstruct->remote;
			Viewers[i].timestamp=GetTickCount();
			Viewers[i].used=false;
			return;
		}
	}

}

void 
Remove_viewer_list(ULONG code)
{
	int i;
	for (i=0;i<20;i++)
	{
		if (Viewers[i].code==code)
		{
			debug("Viewer removed from list %i\n",code);
			Viewers[i].code=0;
			Viewers[i].used=false;
			return;
		}
	}
}

ULONG 
Find_viewer_list(mystruct *Viewerstruct)
{
	int i;
	for (i=0;i<20;i++)
	{
		if (Viewers[i].code==Viewerstruct->code)
		{
			Viewers[i].used=true;
			return i;
		}
	}
	return 99;
}


void
error( const char *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    fprintf(stderr, "ERROR: ");
    vfprintf( stderr, fmt, args );
    va_end( args );
}

int
local_resolve (const char *host, struct sockaddr_in *addr,
	       struct sockaddr_in *ns)
{
    struct hostent *ent;
    if ( strspn(host, "0123456789.") == strlen(host) ) {
	/* given by IPv4 address */
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = inet_addr(host);
    } else {
	debug("resolving host by name: %s\n", host);
	ent = gethostbyname (host);
	if ( ent ) {
	    memcpy (&addr->sin_addr, ent->h_addr, ent->h_length);
	    addr->sin_family = ent->h_addrtype;
	    debug("resolved: %s (%s)\n",
		  host, inet_ntoa(addr->sin_addr));
	} else {
	    debug("failed to resolve locally.\n");
	    return -1;				/* failed */
	}
    }
    return 0;					/* good */
}

SOCKET
open_connection( const char *host, u_short port )
{
    SOCKET s;
    struct sockaddr_in saddr;

    if ( relay_method == METHOD_DIRECT ) {
	host = dest_host;
	port = dest_port;
    }

    if (local_resolve (host, &saddr, NULL) < 0) {
	error("can't resolve hostname: %s\n", host);
	return SOCKET_ERROR;
    }
    saddr.sin_port = htons(port);

    debug("connecting to %s:%u\n", inet_ntoa(saddr.sin_addr), port);
    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( connect( s, (struct sockaddr *)&saddr, sizeof(saddr))
	 == SOCKET_ERROR) {
	debug( "connect() failed.\n");
	return SOCKET_ERROR;
    }
    return s;
}

void
fatal( const char *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    fprintf(stderr, "FATAL: ");
    vfprintf( stderr, fmt, args );
    va_end( args );
    //exit (EXIT_FAILURE);
}

void
report_bytes( char *prefix, char *buf, int len )
{
    if ( ! f_debug )
	return;
    debug( "%s", prefix );
    while ( 0 < len ) {
	fprintf( stderr, " %02x", *(unsigned char *)buf);
	buf++;
	len--;
    }
    fprintf(stderr, "\n");
    return;
}

int
fddatalen( SOCKET fd )
{
    DWORD len = 0;
    struct stat st;
    fstat( 0, &st );
    if ( st.st_mode & _S_IFIFO ) { 
	/* in case of PIPE */
	if ( !PeekNamedPipe( GetStdHandle(STD_INPUT_HANDLE),
			     NULL, 0, NULL, &len, NULL) ) {
	    if ( GetLastError() == ERROR_BROKEN_PIPE ) {
		/* PIPE source is closed */
		/* read() will detects EOF */
		len = 1;
	    } else {
		fatal("PeekNamedPipe() failed, errno=%d\n",
		      GetLastError());
	    }
	}
    } else if ( st.st_mode & _S_IFREG ) {
	/* in case of regular file (redirected) */
	len = 1;			/* always data ready */
    } else if ( _kbhit() ) {
	/* in case of console */
	len = 1;
    }
    return len;
}



DWORD WINAPI do_repeater(LPVOID lpParam)
{
    /** vars for local input data **/
    char lbuf[1024];				/* local input buffer */
    int lbuf_len;				/* available data in lbuf */
    int f_local;				/* read local input more? */
    /** vars for remote input data **/
    char rbuf[1024];				/* remote input buffer */
    int rbuf_len;				/* available data in rbuf */
    int f_remote;				/* read remote input more? */
    /** other variables **/
    int nfds, len;
    fd_set *ifds, *ofds;
    struct timeval *tmo;
    struct timeval win32_tmo;
	
	SOCKET local_in=0;
	SOCKET local_out=0;
	SOCKET remote=0;
	ULONG code=0;
	pmystruct inout=(pmystruct)lpParam;
    local_in=inout->local_in;
	local_out=inout->local_out;
	remote=inout->remote;
	code=inout->code;
    /* repeater between stdin/out and socket  */
    nfds = ((local_in<remote)? remote: local_in) +1;
    ifds = FD_ALLOC(nfds);
    ofds = FD_ALLOC(nfds);
    f_local = 1;				/* yes, read from local */
    f_remote = 1;				/* yes, read from remote */
    lbuf_len = 0;
    rbuf_len = 0;

    while ( f_local || f_remote ) {
	FD_ZERO( ifds );
	FD_ZERO( ofds );
	tmo = NULL;

	/** prepare for reading local input **/
	if ( f_local && (lbuf_len < sizeof(lbuf)) ) {
	    if ( local_type != LOCAL_SOCKET ) {
		/* select() on Winsock is not accept standard handle.
		   So use select() with short timeout and checking data
		   in stdin by another method. */
		win32_tmo.tv_sec = 0;
		win32_tmo.tv_usec = 10*1000;	/* 10 ms */
		tmo = &win32_tmo;
	    } else
	    FD_SET( local_in, ifds );
	}
	
	/** prepare for reading remote input **/
	if ( f_remote && (rbuf_len < sizeof(rbuf)) ) {
	    FD_SET( remote, ifds );
	}
	
	/* FD_SET( local_out, ofds ); */
	/* FD_SET( remote, ofds ); */
	
	if ( select( nfds, ifds, ofds, NULL, tmo ) == -1 ) {
	    /* some error */
	    error( "select() failed, %d\n", socket_errno());
		Remove_server_list(code);
		Remove_viewer_list(code);
	    return -1;
	}
	/* fake ifds if local is stdio handle because
           select() of Winsock does not accept stdio
           handle. */
	if (f_local && (local_type!=LOCAL_SOCKET) && (0<fddatalen(local_in)))
	    FD_SET(0,ifds);		/* data ready */

	/* remote => local */
	if ( FD_ISSET(remote, ifds) && (rbuf_len < sizeof(rbuf)) ) {
	    len = recv( remote, rbuf + rbuf_len, sizeof(rbuf)-rbuf_len, 0);
	    if ( len == 0 ) {
		debug("connection closed by peer\n");
		f_remote = 0;			/* no more read from socket */
		f_local = 0;
		closesocket(local_in);
		closesocket(remote);
		shutdown(local_in, 1);
		shutdown(remote, 1);
		local_in=0;
		remote=0;

	    } else if ( len == -1 ) {
		if (socket_errno() != ECONNRESET) {
		    /* error */
		    fatal("recv() faield, %d\n", socket_errno());
		} else {
		    debug("ECONNRESET detected\n");
		}
	    } else {
		//debug("recv %d bytes\n", len);
		if ( 1 < f_debug )		/* more verbose */
		    report_bytes( "<<<", rbuf, rbuf_len);
		rbuf_len += len;
	    }
	}
	
	/* local => remote */
	if ( FD_ISSET(local_in, ifds) && (lbuf_len < sizeof(lbuf)) ) {
	    if (local_type == LOCAL_SOCKET)
		len = recv(local_in, lbuf + lbuf_len,
			   sizeof(lbuf)-lbuf_len, 0);
	    else
		len = read(local_in, lbuf + lbuf_len, sizeof(lbuf)-lbuf_len);
	    if ( len == 0 ) {
		/* stdin is EOF */
		debug("local input is EOF\n");
		closesocket(local_in);
		closesocket(remote);
		shutdown(local_in, 1);
		shutdown(remote, 1);
		local_in=0;
		remote=0;
		f_local = 0;
	    } else if ( len == -1 ) {
		/* error on reading from stdin */
		debug("recv() failed, errno = %d\n", errno);
		Remove_server_list(code);
		Remove_viewer_list(code);
		return 0;
	    } else {
		/* repeat */
		lbuf_len += len;
	    }
	}
	
	/* flush data in buffer to socket */
	if ( 0 < lbuf_len ) {
	    len = send(remote, lbuf, lbuf_len, 0);
	    if ( 1 < f_debug )		/* more verbose */
		report_bytes( ">>>", lbuf, lbuf_len);
	    if ( len == -1 ) {
		debug("send() failed, %d\n", socket_errno());
		//return 0;
	    } else if ( 0 < len ) {
		/* move data on to top of buffer */
		//debug("send %d bytes\n", len);
		lbuf_len -= len;
		if ( 0 < lbuf_len )
		    memcpy( lbuf, lbuf+len, lbuf_len );
		assert( 0 <= lbuf_len );
	    }
	}
	
	/* flush data in buffer to local output */
	if ( 0 < rbuf_len ) {
	    if (local_type == LOCAL_SOCKET)
		len = send( local_out, rbuf, rbuf_len, 0);
	    else
		len = write( local_out, rbuf, rbuf_len);
	    if ( len == -1 ) {
		debug("output (local) failed, errno=%d\n", errno);
		//return 0;
	    } 
		else
		{
	    rbuf_len -= len;
	    if ( len < rbuf_len )
		memcpy( rbuf, rbuf+len, rbuf_len );
	    assert( 0 <= rbuf_len );
		}
	}

    }
    Remove_server_list(code);
	Remove_viewer_list(code);
    return 0;
}

BOOL ParseDisplay(LPTSTR display, LPTSTR phost, int hostlen, int *pport) 
{
	int tmp_port;
	TCHAR *colonpos = _tcschr(display, L':');
    if (hostlen < (int)_tcslen(display))
        return FALSE;

    if (colonpos == NULL)
	{
		// No colon -- use default port number
        tmp_port = RFB_PORT_OFFSET;
		_tcsncpy(phost, display, MAX_HOST_NAME_LEN);
	}
	else
	{
		_tcsncpy(phost, display, colonpos - display);
		phost[colonpos - display] = L'\0';
		if (colonpos[1] == L':') {
			// Two colons -- interpret as a port number
			if (_stscanf(colonpos + 2, TEXT("%d"), &tmp_port) != 1) 
				return FALSE;
		}
		else
		{
			// One colon -- interpret as a display number or port number
			if (_stscanf(colonpos + 1, TEXT("%d"), &tmp_port) != 1) 
				return FALSE;

			// RealVNC method - If port < 100 interpret as display number else as Port number
			if (tmp_port < 100)
				tmp_port += RFB_PORT_OFFSET;
		}
	}
    *pport = tmp_port;
    return TRUE;
}

int
WriteExact(int sock, char *buf, int len)
{
    int n;
	
    while (len > 0) {
	n = send(sock, buf, len,0);
		
	if (n > 0) {
	    buf += n;
	    len -= n;
	} else if (n == 0) {
      	    fprintf(stderr,"WriteExact: write returned 0?\n");
	    exit(1);
        } else {
            return n;
	}
    }
    return 1;
}

int ReadExact(int sock, char *buf, int len)
{
    int n;
 
    while (len > 0) {
	n = recv(sock, buf, len, 0);
	if (n > 0) {
	    buf += n;
	    len -= n;
        } else {
            return n;
	}
    }
    return 1;
}


DWORD WINAPI server_listen(LPVOID lpParam)
{
    int ret;
    SOCKET  remote;				/* socket */
    SOCKET  local_in;				/* Local input */
    SOCKET  local_out;				/* Local output */
    
    int sock;
    int connection;
    struct sockaddr_in name;
    struct sockaddr client;
    int socklen;
	DWORD dwThreadId;
	TCHAR proxyadress[256];
	TCHAR remotehost[256];
	int remoteport;
	mystruct teststruct;
	ULONG code;
	
	f_debug=1;


	////////////
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) fatal("socket() failed, errno=%d\n", socket_errno());
	else debug("socket() initialized\n");
    name.sin_family = AF_INET;
    name.sin_port = htons (server_port);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
			fatal ("bind() failed, errno=%d\n", socket_errno());
	else debug ("bind() succeded to port %i\n",server_port);
    if (listen( sock, 1) < 0)
		fatal ("listen() failed, errno=%d\n", socket_errno());
	else debug ("listen() succeded\n");
    socklen = sizeof(client);
	while(notstopped)
	{
		debug ("Waiting for connection ...\n");
		connection = accept( sock, &client, &socklen);
		if ( connection < 0 )
			debug ("accept() failed, errno=%d\n", socket_errno());
		else
		{
			if (ReadExact(connection, proxyadress, MAX_HOST_NAME_LEN)<0){
				debug("Reading Proxy settings error");
				closesocket(connection);
				goto end;
				}
			ParseDisplay(proxyadress, remotehost, 255, &remoteport);
			strcpy(dest_host,remotehost);
			dest_port=remoteport;
			shutdown(sock, 2);
			local_in = local_out=connection;
			if (strcmp(dest_host,"ID")==0)
			{
				teststruct.remote=connection;
				teststruct.code=remoteport;
				Add_server_list(&teststruct);
				if (Find_viewer_list(&teststruct)!=99)
				{
					//found
					//initiate connection
					Find_server_list(&teststruct);
					teststruct.local_in=Viewers[Find_viewer_list(&teststruct)].local_in;
					teststruct.local_out=Viewers[Find_viewer_list(&teststruct)].local_out;
					teststruct.remote=connection;
					CreateThread(NULL,0,do_repeater,(LPVOID)&teststruct,0,&dwThreadId);
				}
			}
		}
end:;
   
    }
	notstopped=false;
    debug ("relaying done.\n");
    closesocket(remote);

	closesocket(local_in);
    WSACleanup();
    return 0;
}

DWORD WINAPI timer(LPVOID lpParam)
{
	while(notstopped)
	{
		int i;
		DWORD tick=GetTickCount();
		for (i=0;i<20;i++)
			{
				if (tick-Viewers[i].timestamp>300000 && Viewers[i].used==false && Viewers[i].code!=0)
				{
					//
					closesocket(Viewers[i].local_in);
					debug("Remove viewer %i %i \n",Viewers[i].code,i);
					Remove_viewer_list(Viewers[i].code);
				}
				if (tick-Servers[i].timestamp>300000 && Servers[i].used==false && Servers[i].code!=0)
				{
					//
					closesocket(Servers[i].remote);
					debug("Remove server %i\n",Servers[i].code);
					Remove_server_list(Servers[i].code);
				}
			}
		debug("Searching old connections\n");
		Sleep(60000);

	}
	return 0;
}

int
main( int argc, char **argv )
{
    int ret;
    SOCKET  remote;				/* socket */
    SOCKET  local_in;				/* Local input */
    SOCKET  local_out;				/* Local output */
    WSADATA wsadata;
	TCHAR proxyadress[256];
	TCHAR remotehost[256];
	int remoteport;
    int sock;
    int connection;
    struct sockaddr_in name;
    struct sockaddr client;
    int socklen;
	DWORD dwThreadId;
	mystruct teststruct;
	rfbProtocolVersionMsg pv;
	f_debug=1;
	local_type = LOCAL_SOCKET;
	notstopped=true;

    WSAStartup( 0x101, &wsadata);
	relay_method = METHOD_DIRECT;
	Clean_server_List();
	Clean_viewer_List();
	////////////
	local_port=5900;
	server_port=5500;
	if (argc==2)
		local_port=atoi(argv[1]);
	if (argc==3)
		server_port=atoi(argv[2]);
	dest_host =malloc(256);
	////////////
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) fatal("socket() failed, errno=%d\n", socket_errno());
	else debug("socket() initialized\n");
    name.sin_family = AF_INET;
    name.sin_port = htons (local_port);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
			fatal ("bind() failed, errno=%d\n", socket_errno());
	else debug ("bind() succeded to port %i\n",local_port);
    if (listen( sock, 1) < 0)
		fatal ("listen() failed, errno=%d\n", socket_errno());
	else debug ("listen() succeded\n");
    socklen = sizeof(client);
	CreateThread(NULL,0,server_listen,(LPVOID)&teststruct,0,&dwThreadId);
	CreateThread(NULL,0,timer,(LPVOID)&teststruct,0,&dwThreadId);
	while(notstopped)
	{
		debug ("Waiting for Viewer connection ...\n");
		connection = accept( sock, &client, &socklen);
		if ( connection < 0 )
			debug ("accept() failed, errno=%d\n", socket_errno());
		else
		{
			debug ("accept() connection \n");
			sprintf(pv,rfbProtocolVersionFormat,rfbProtocolMajorVersion,rfbProtocolMinorVersion);
			if (WriteExact(connection, pv, sz_rfbProtocolVersionMsg) < 0) {
				debug("Writing protocol version error");
				closesocket(connection);
				goto end;
				}
			if (ReadExact(connection, proxyadress, MAX_HOST_NAME_LEN)<0){
				debug("Reading Proxy settings error");
				closesocket(connection);
				goto end;
				}
			ParseDisplay(proxyadress, remotehost, 255, &remoteport);
			strcpy(dest_host,remotehost);
			dest_port=remoteport;
			//debug ("Server %s port %d \n", dest_host,remoteport);
			shutdown(sock, 2);
			local_in = local_out=connection;
			if (strcmp(dest_host,"ID")==0)
			{
				teststruct.local_in=local_in;
				teststruct.local_out=local_out;
				teststruct.code=remoteport;
				Add_viewer_list(&teststruct);

				if (Find_server_list(&teststruct)!=99)
				{
					//found
					//initiate connection
					Find_viewer_list(&teststruct);
					teststruct.local_in=local_in;
					teststruct.local_out=local_out;
					teststruct.remote=Servers[Find_server_list(&teststruct)].remote;
					CreateThread(NULL,0,do_repeater,(LPVOID)&teststruct,0,&dwThreadId);
				}

				//CreateThread(NULL,0,do_repeater_listen,(LPVOID)&teststruct,0,&dwThreadId);
			}
			else
			{
			remote = open_connection (dest_host, dest_port);
				if ( remote == SOCKET_ERROR )
				{
					debug( "Unable to connect to destination host, errno=%d\n",socket_errno());
					goto end;
				}
			debug("connected\n");
			debug ("start relaying.\n");
			teststruct.local_in=local_in;
			teststruct.local_out=local_out;
			teststruct.remote=remote;
			CreateThread(NULL,0,do_repeater,(LPVOID)&teststruct,0,&dwThreadId);
			}
				//do_repeater(local_in, local_out, remote);
		}
end:;
   
    }
	notstopped=false;
    debug ("relaying done.\n");
    closesocket(remote);

	closesocket(local_in);
    WSACleanup();
    return 0;
}