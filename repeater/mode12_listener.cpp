#include "repeater.h"

void error( const char *fmt, ... );

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
	char remote_host[1024];
	BOOL found;
	int i;
    SOCKET s;
    struct sockaddr_in saddr;

    if (local_resolve (host, &saddr, NULL) < 0) {
	error("can't resolve hostname: %s\n", host);
	return SOCKET_ERROR;
    }
    saddr.sin_port = htons(port);

    debug("connecting to %s:%u\n", inet_ntoa(saddr.sin_addr), port);
	strcpy(remote_host,inet_ntoa(saddr.sin_addr));
	found=FALSE;
	if (!saved_allow) found=TRUE;
	else
	{
		for (i=0;i<rule1;i++)
		{
		if (strstr(temp1[i],remote_host) != NULL)
		found=TRUE;
		}
	}
	if (saved_refuse)
	{
		for (i=0;i<rule2;i++)
		{
		if (strstr(temp2[i],inet_ntoa(saddr.sin_addr)) != NULL)
		found=FALSE;
		}
	}
	if (!found) return SOCKET_ERROR;
    s = socket( AF_INET, SOCK_STREAM, 0 );
	// NAGLE
	const int one = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));

    if ( connect( s, (struct sockaddr *)&saddr, sizeof(saddr))
	 == SOCKET_ERROR) {
	debug( "connect() failed.\n");
	return SOCKET_ERROR;
    }
    return s;
}

HANDLE hmode12listener=NULL;
int sock=0;

DWORD WINAPI mode12listener(LPVOID lpParam)
	{
		TCHAR dest_host[256]="";
		u_short dest_port;
		struct sockaddr_in name;
		struct sockaddr_in client;
		int socklen;
		const int one = 1;
		int connection;
		rfbProtocolVersionMsg pv;
		TCHAR proxyadress[256]="";
		TCHAR remotehost[256]="";
		int remoteport;
		SOCKET  local_in;				/* Local input */
		SOCKET  local_out;				/* Local output */
		SOCKET  remote;
		mystruct teststruct;
		DWORD dwThreadId;
		BOOL found2;
		int i;
		char *ip_peer;
		
		sock = socket (PF_INET, SOCK_STREAM, 0);
		if (sock < 0) fatal("socket() failed, errno=%d\n", socket_errno());
		else debug("socket() initialized\n");
	
		setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));

		name.sin_family = AF_INET;
		name.sin_port = htons (saved_portA);
		name.sin_addr.s_addr = htonl (INADDR_ANY);
		if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
				fatal ("bind() failed, errno=%d\n", socket_errno());
		else debug ("bind() succeded to port %i\n",saved_portA);
		if (listen( sock, 1) < 0)
			fatal ("listen() failed, errno=%d\n", socket_errno());
		else debug ("listen() succeded\n");
	    socklen = sizeof(client);

		while(notstopped)
		{
			memset(dest_host,0,256);
			memset(proxyadress,0,256);
			memset(remotehost,0,256);
			memset(pv,0,sizeof(pv));
	//		debug ("Waiting for Viewer connection ...\n");
			connection = accept( sock, (struct sockaddr*)&client, &socklen);
			Sleep(1000);
			ip_peer=inet_ntoa(client.sin_addr);
			if ( connection < 0 )
			{
				debug ("accept() failed, errno=%d\n", socket_errno());
				debug ("Check if port is not already in use port=%i ",saved_portA);
				Sleep(500);
			}
			else
			{
				// NAGLE
				const int one_local = 1;
				setsockopt(connection, IPPROTO_TCP, TCP_NODELAY, (char *)&one_local, sizeof(one_local));
				DWORD ii = 5000;  // disable
				setsockopt (connection, SOL_SOCKET, SO_RCVTIMEO, (char*) &ii, sizeof(ii));
				setsockopt (connection, SOL_SOCKET, SO_SNDTIMEO, (char*) &ii, sizeof(ii));

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
				DWORD i = 0;  // disable
				setsockopt (connection, SOL_SOCKET, SO_RCVTIMEO, (char*) &i, sizeof(i));
				setsockopt (connection, SOL_SOCKET, SO_SNDTIMEO, (char*) &i, sizeof(i));
				if (!ParseDisplay(proxyadress, remotehost, 255, &remoteport))
					{
					debug("ParseDisplay failed");
					shutdown(sock, 2);
					closesocket(connection);
					continue;
					}
				strcpy(dest_host,remotehost);
				dest_port=remoteport;
				//debug ("Server %s port %d \n", dest_host,remoteport);
				shutdown(sock, 2);
				local_in = local_out=connection;
				char * pch;
				char * comm;
				pch = strtok (dest_host,"+");
				bool alsoID=false;
				if (pch != NULL && saved_usecom)
				{
					if (strcmp(pch,"ID")==0) alsoID=true;
					comm = strtok (NULL, ":");
					if (comm)
						{
							if (lookup_comment(remoteport)==NULL)
							{
								closesocket(connection);
								debug("ID-XXX refused %i , ID not in use.\n",remoteport,comm);
								goto end;
							}
							if (strcmp(lookup_comment(remoteport),comm)!=0)
							{
								closesocket(connection);
								debug("ID-XXX refused %i , incorrect %s\n",remoteport,comm);
								goto end;
							}
						}
					else
					{
						closesocket(connection);
						debug("ID- refused %i\n",remoteport);
						goto end;
					}
				}
				else if (pch == NULL && saved_usecom)
				{
					closesocket(connection);
					debug("ID refused %i \n",remoteport);
					goto end;
				}

				//

				if (strcmp(dest_host,"ID")==0 || alsoID)
					{
						found2=TRUE;
						if (saved_refuse2)
						{
							found2=FALSE;
							for (i=0;i<rule3;i++)
								{
									if (atoi(temp3[i])==remoteport) found2=1;

								}
						}
						if (!found2)
						{
							closesocket(connection);
							debug("ID refused %i \n",remoteport);
						}
						if (found2)
						{
							teststruct.local_in=local_in;
							teststruct.local_out=local_out;
							teststruct.code=remoteport;
							strcpy_s(teststruct.hostname,ip_peer);
							Add_viewer_list(&teststruct);
							debug("ID added %i \n",remoteport);
					

							if (Find_server_list(&teststruct)!=MAX_LIST+1)
							{
								//found
								//initiate connection
								Find_viewer_list(&teststruct);
								teststruct.local_in=local_in;
								teststruct.local_out=local_out;
								teststruct.remote=Servers[Find_server_list(&teststruct)].remote;
								teststruct.server=0;
								teststruct.server_nummer=Find_server_list(&teststruct);
								teststruct.viewer_nummer=Find_viewer_list(&teststruct);
								CreateThread(NULL,0,do_repeater,(LPVOID)&teststruct,0,&dwThreadId);
							}
							else
							{
								if (Find_viewer_list_id(&teststruct)!=MAX_LIST+1)
								{
									teststruct.server=0;
									teststruct.remote=local_in;
									if (teststruct.waitingThread==0)CreateThread(NULL,0,do_repeater_wait,(LPVOID)&teststruct,0,&dwThreadId);
								}
							}
						}
					}
				else if (saved_mode1)
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
		notstopped=0;
		debug ("relaying done mode12listener.\n");
		return 0;
}

void Start_mode12listenerThread()
{
	notstopped=1;
	DWORD iID;
	hmode12listener=CreateThread(NULL, 0, mode12listener, NULL, 0, &iID);
}

void Stop_mode12listenerThread()
{
	closesocket(sock);
	notstopped=0;

	WaitForSingleObject(hmode12listener,5000);
	CloseHandle(hmode12listener);
}

