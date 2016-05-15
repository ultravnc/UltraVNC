#include "repeater.h"
int f_debug=1;

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
	}	else
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

BOOL ParseDisplay2(LPTSTR display, LPTSTR phost, int hostlen, char pport[32]) 
{
	//char tmp_port[512];
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
    strncpy(pport,_itoa(tmp_port,pport,10),32);
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
      	     return -1;
    } else {
            return n;
	}
    }
    return 1;
}

int ReadExact(int sock, char *buf, int len)
{
    int n;
	Sleep(500);
    while (len > 0) {
	n = recv(sock, buf, len, 0);
	if (n > 0) {
	    buf += n;
	    len -= n;
		if (len!=0) return -1;
        } else {
            return n;
	}
    }
    return 1;
}


DWORD WINAPI do_repeater_wait(LPVOID lpParam)
{
    /** vars for remote input data **/
	int a;
    //char rbuf[1025];				/* remote input buffer */
	char *rbuf;
    int rbuf_len;				/* available data in rbuf */
    int f_remote;				/* read remote input more? */
    /** other variables **/
    int len;
	bool sendkepalive=false;
	int sendkepalive_counter=0;
	
	//SOCKET local_in=0;
	SOCKET local_out=0;
	SOCKET remote=0;
	ULONG code=0;
	long recvbytes=0;
	long sendbytes=0;
	BOOL server;
	int nummer;
	pmystruct inout=(pmystruct)lpParam;
	remote=inout->remote;
	code=inout->code;
	server=inout->server;
	//nummer is the correct nummer for server or viewer, depend on th switch
	nummer=inout->nummer;
	if (server) 
	{
		rbuf=Servers[nummer].preloadbuffer;
		Servers[nummer].waitingThread=1;
		LogStats_server(code,nummer);
		sendkepalive=true;
	}
	else
	{
		rbuf=Viewers[nummer].preloadbuffer;
		Viewers[nummer].waitingThread=1;
		LogStats_viewer(code,nummer);
	}
    
    
    f_remote = 1;				/* yes, read from remote */
    rbuf_len = 0;

	while (f_remote && notstopped  && Servers[nummer].code!=999999999) {
	FD_SET ifds;
	struct timeval tmo;
	FD_ZERO( &ifds );
	tmo.tv_sec=1;
	tmo.tv_usec=0;

	if (server)
	{
		if (Servers[nummer].used)
		{
			Servers[nummer].waitingThread=0;
			return 0;
		}
	}
	else
	{
		if (Viewers[nummer].used)
		{
			Viewers[nummer].waitingThread=0;
			return 0;
		}
	}	 

	FD_SET( remote, &ifds );
	a=select( 0, &ifds, NULL, NULL, &tmo );
	if ( a == 0 ) {
		if (server && sendkepalive && saved_keepalive) sendkepalive_counter++;
		if (sendkepalive_counter==60 && sendkepalive && server && saved_keepalive)
		{
			sendkepalive_counter=0;
			//Beep(1000,1000);
			rfbProtocolVersionMsg pv;
			sprintf(pv,rfbProtocolKeepAlive,rfbProtocolMajorVersion,rfbProtocolMinorVersion);
 			if (WriteExact(remote, pv, sz_rfbProtocolVersionMsg) < 0) goto error;
			recvbytes=recvbytes-12;
			rbuf_len=rbuf_len-12;
		}
	    /* some error */
	    //debug( "select() 0 \n");
	}
	if ( a == -1 ) {
	    /* some error */
	    debug( "select() failed, %d\n", socket_errno());
		goto error;
	}
	if ( a == 1)
	{
		if (rbuf_len >1024) goto error;
		if ( FD_ISSET(remote, &ifds) && (rbuf_len < 1025) ) 
		{
			len = recv( remote, rbuf + rbuf_len, 1024-rbuf_len, 0);
			debug( "recv %d \n",len);
			if ( len == 0 ) {
			debug("connection closed by peer\n");
			goto error;

			} else if ( len == -1 ) {
			if (socket_errno() != ECONNRESET) {
				
				fatal("recv() faield, %d\n", socket_errno());
				goto error;
			} else {
				debug("ECONNRESET detected\n");
				goto error;
			}
			} else {
			recvbytes +=len;
			if ( 1 < f_debug )		
				report_bytes( "<<<", rbuf, rbuf_len);
			rbuf_len += len;
			if (server) 
				{
					if (len!=12) sendkepalive=false;
					Servers[nummer].timestamp=GetTickCount();
					Servers[nummer].size_buffer=rbuf_len;
					char	msg[100];
					char	buf[5];
					SYSTEMTIME	st; 
					GetLocalTime(&st);
					_itoa(st.wYear,buf,10);
					strcpy(msg,buf);
					strcat(msg,"/");
					_itoa(st.wMonth,buf,10);
					strcat(msg,buf);
					strcat(msg,"/");
					_itoa(st.wDay,buf,10);
					strcat(msg,buf);
					strcat(msg," ");
					_itoa(st.wHour,buf,10);
					strcat(msg,buf);
					strcat(msg,":");
					_itoa(st.wMinute,buf,10);
					strcat(msg,buf);
					strcat(msg,":");
					_itoa(st.wSecond,buf,10);
					strcat(msg,buf);
					strcpy_s(Servers[nummer].time,msg);

				}
				else
				{
					Viewers[nummer].size_buffer=rbuf_len;
				}
			}
		}
	}
	
	
	}
error:
	//LogStats(code,recvbytes,sendbytes);
	f_remote = 0;			/* no more read from socket */
	shutdown(remote, 1);
	closesocket(remote);	
	//local_in=0;
	remote=0;
    if (server)
	{
		Servers[nummer].waitingThread=0;
		Remove_server_list(Servers[nummer].code);
	}
	else
	{
		Viewers[nummer].waitingThread=0;
		Remove_viewer_list(Viewers[nummer].code);
	}
    return 0;
}

DWORD WINAPI do_repeater(LPVOID lpParam)
{
	char	start_msg[100];
	char	stop_msg[100];
	char	buf[5];
			

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
//    struct timeval win32_tmo;
	
	SOCKET local_in=0;
	SOCKET local_out=0;
	SOCKET remote=0;
	ULONG code=0;
	//long recvbytes=0;
	//long sendbytes=0;
	int nummer;
	int viewer_nummer;
	int server_nummer;
	pmystruct inout=(pmystruct)lpParam;
    local_in=inout->local_in;
	local_out=inout->local_out;
	remote=inout->remote;
	code=inout->code;
	nummer=inout->nummer;
	viewer_nummer=inout->viewer_nummer;
	server_nummer=inout->server_nummer;
	SYSTEMTIME	st; 
	GetLocalTime(&st);
	_itoa(st.wYear,buf,10);
	strcpy(start_msg,buf);
	strcat(start_msg,"/");
	_itoa(st.wMonth,buf,10);
	strcat(start_msg,buf);
	strcat(start_msg,"/");
	_itoa(st.wDay,buf,10);
	strcat(start_msg,buf);
	strcat(start_msg," ");
	_itoa(st.wHour,buf,10);
	strcat(start_msg,buf);
	strcat(start_msg,":");
	_itoa(st.wMinute,buf,10);
	strcat(start_msg,buf);
	strcat(start_msg,":");
	_itoa(st.wSecond,buf,10);
	strcat(start_msg,buf);
	strcat(start_msg," ");

	lbuf_len = 0;
    rbuf_len = 0;
	Viewers[viewer_nummer].recvbytes=0;
	Viewers[viewer_nummer].sendbytes=0;
	if (inout->server) 
	{
		memcpy(lbuf,Viewers[viewer_nummer].preloadbuffer,Viewers[viewer_nummer].size_buffer);
		lbuf_len=Viewers[viewer_nummer].size_buffer;
		Viewers[viewer_nummer].size_buffer=0;
		LogStats_server(code,viewer_nummer);
	}
	else
	{
		memcpy(lbuf,Servers[server_nummer].preloadbuffer,Servers[server_nummer].size_buffer);
		lbuf_len=Servers[server_nummer].size_buffer;
		Servers[server_nummer].size_buffer=0;
		LogStats_viewer(code,server_nummer);
	}
	if ( 0 < lbuf_len ) {
	    if (inout->server) len = send(remote, lbuf, lbuf_len, 0);
		else len = send(local_in, lbuf, lbuf_len, 0);
	    if ( 1 < f_debug )		/* more verbose */
		report_bytes( ">>>", lbuf, lbuf_len);
	    if ( len == -1 ) {
		debug("send() failed, %d\n", socket_errno());
		goto error;
	    } else if ( 0 < len ) {
		/* move data on to top of buffer */
		Viewers[viewer_nummer].sendbytes+=len;
		lbuf_len -= len;
		if ( 0 < lbuf_len )
		    memcpy( lbuf, lbuf+len, lbuf_len );
		assert( 0 <= lbuf_len );
	    }
	}
	
    /* repeater between stdin/out and socket  */
    nfds = ((local_in<remote)? remote: local_in) +1;
    ifds = FD_ALLOC(nfds);
    ofds = FD_ALLOC(nfds);
    f_local = 1;				/* yes, read from local */
    f_remote = 1;				/* yes, read from remote */
	DWORD start=0;
	DWORD stop=0;
	int measure_counter=0;
	long temp_bytes=0;

    while ( f_local || f_remote ) {
	if (measure_counter==0) start=timeGetTime();
	 measure_counter++;
	FD_ZERO( ifds );
	FD_ZERO( ofds );
	tmo = NULL;

	/** prepare for reading local input **/
	if ( f_local && (lbuf_len < sizeof(lbuf)) ) {
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
		goto error;
	}
	/* fake ifds if local is stdio handle because
           select() of Winsock does not accept stdio
           handle. */

	/* remote => local */
	if ( FD_ISSET(remote, ifds) && (rbuf_len < sizeof(rbuf)) ) {
	    len = recv( remote, rbuf + rbuf_len, sizeof(rbuf)-rbuf_len, 0);
	    if ( len == 0 ) {
		debug("connection closed by peer\n");
		goto error;

	    } else if ( len == -1 ) {
		if (socket_errno() != ECONNRESET) {
		    /* error */
		    fatal("recv() faield, %d\n", socket_errno());
			goto error;
		} else {
		    debug("ECONNRESET detected\n");
			goto error;
		}
	    } else {
		Viewers[viewer_nummer].recvbytes +=len;
		temp_bytes+=len;
		if ( 1 < f_debug )		/* more verbose */
		    report_bytes( "<<<", rbuf, rbuf_len);
		rbuf_len += len;
	    }
	}
	
	/* local => remote */
	if ( FD_ISSET(local_in, ifds) && (lbuf_len < sizeof(lbuf)) ) {

		len = recv(local_in, lbuf + lbuf_len,sizeof(lbuf)-lbuf_len, 0);

	    if ( len == 0 ) {
		/* stdin is EOF */
		debug("local input is EOF\n");
		goto error;
	    } else if ( len == -1 ) {
		/* error on reading from stdin */			goto error;

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
		goto error;
	    } else if ( 0 < len ) {
		/* move data on to top of buffer */
		Viewers[viewer_nummer].sendbytes+=len;
		temp_bytes+=len;
		lbuf_len -= len;
		if ( 0 < lbuf_len )
		    memcpy( lbuf, lbuf+len, lbuf_len );
		assert( 0 <= lbuf_len );
	    }
	}
	
	/* flush data in buffer to local output */
	if ( 0 < rbuf_len ) {

		len = send( local_out, rbuf, rbuf_len, 0);
	    if ( len == -1 ) {
		debug("output (local) failed, errno=%d\n", errno);
		goto error;
	    } 
		else
		{
	    rbuf_len -= len;
	    if ( len < rbuf_len )
		memcpy( rbuf, rbuf+len, rbuf_len );
	    assert( 0 <= rbuf_len );
		}
	}
	if (measure_counter==10) 
		{
			stop=timeGetTime();
			measure_counter=0;
			if ((stop-start)>0) Viewers[nummer].average=temp_bytes*1000/(stop-start)/1000;
			temp_bytes=0;
		}

    }
error:
	GetLocalTime(&st);
	_itoa(st.wYear,buf,10);
	strcpy(stop_msg,buf);
	strcat(stop_msg,"/");
	_itoa(st.wMonth,buf,10);
	strcat(stop_msg,buf);
	strcat(stop_msg,"/");
	_itoa(st.wDay,buf,10);
	strcat(stop_msg,buf);
	strcat(stop_msg," ");
	_itoa(st.wHour,buf,10);
	strcat(stop_msg,buf);
	strcat(stop_msg,":");
	_itoa(st.wMinute,buf,10);
	strcat(stop_msg,buf);
	strcat(stop_msg,":");
	_itoa(st.wSecond,buf,10);
	strcat(stop_msg,buf);
	strcat(stop_msg," ");
	LogStats_access(start_msg,stop_msg,code,viewer_nummer,server_nummer,Viewers[viewer_nummer].sendbytes+Viewers[viewer_nummer].recvbytes);
	//LogStats(code,recvbytes,sendbytes);
	f_remote = 0;			/* no more read from socket */
	f_local = 0;
	shutdown(local_in, 1);
	shutdown(remote, 1);
	closesocket(local_in);
	closesocket(remote);
	local_in=0;
	remote=0;
	Remove_server_list(Servers[server_nummer].code);
	Remove_viewer_list(Viewers[viewer_nummer].code);
    return 0;
}
