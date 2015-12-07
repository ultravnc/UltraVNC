#include "repeater.h"

HANDLE hserver_listen=NULL;
int sock_server_listen=NULL;

DWORD WINAPI server_listen(LPVOID lpParam)
{
	TCHAR dest_host[256];
	u_short dest_port;
    SOCKET  local_in=INVALID_SOCKET;				/* Local input */
    SOCKET  local_out=INVALID_SOCKET;				/* Local output */
    
    int connection;
    struct sockaddr_in name;
    struct sockaddr_in client;
    int socklen;
	BOOL found2;
	DWORD dwThreadId;
	TCHAR proxyadress[256];
	TCHAR remotehost[256];
	int remoteport;
	mystruct teststruct;
	char *ip_peer;



	////////////
    sock_server_listen = socket (PF_INET, SOCK_STREAM, 0);
    if (sock_server_listen < 0) fatal("socket() failed, errno=%d\n", socket_errno());
	else debug("socket() initialized\n");
	// NAGLE
	const int one = 1;
	setsockopt(sock_server_listen, IPPROTO_TCP, TCP_NODELAY, (char *)&one, sizeof(one));

    name.sin_family = AF_INET;
    name.sin_port = htons (saved_portB);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock_server_listen, (struct sockaddr *) &name, sizeof (name)) < 0)
			fatal ("bind() failed, errno=%d\n", socket_errno());
	else debug ("bind() succeded to port %i\n",saved_portB);
    if (listen( sock_server_listen, 1) < 0)
		fatal ("listen() failed, errno=%d\n", socket_errno());
	else debug ("listen() succeded\n");
    socklen = sizeof(client);
	while(notstopped)
	{	
		memset(dest_host,0,256);
		memset(proxyadress,0,256);
		memset(remotehost,0,256);

		connection = accept( sock_server_listen, (struct sockaddr*)&client, &socklen);
		Sleep(1000);
		ip_peer=inet_ntoa(client.sin_addr);

		if ( connection < 0 )
		{
			debug ("accept() failed, errno=%d\n", socket_errno());
			debug ("Check if port is not already in use port=%i ",saved_portB);
			Sleep(500);
		}
		else
		{
			// NAGLE
			const int one_local = 1;
			setsockopt(connection, IPPROTO_TCP, TCP_NODELAY, (char *)&one_local, sizeof(one_local));
			setsockopt(connection, SOL_SOCKET, SO_KEEPALIVE, (char *)&one_local, sizeof(one_local));
			int ii = 5000;  // disable
			setsockopt (connection, SOL_SOCKET, SO_RCVTIMEO, (char*) &ii, sizeof(ii));
			setsockopt (connection, SOL_SOCKET, SO_SNDTIMEO, (char*) &ii, sizeof(ii));

			if (ReadExact(connection, proxyadress, MAX_HOST_NAME_LEN)<0){
				debug("Reading Proxy settings error");
				closesocket(connection);
				goto end;
				}
				int i = 0;  // disable
				setsockopt (connection, SOL_SOCKET, SO_RCVTIMEO, (char*) &i, sizeof(i));
				setsockopt (connection, SOL_SOCKET, SO_SNDTIMEO, (char*) &i, sizeof(i));
			if (!ParseDisplay(proxyadress, remotehost, 255, &remoteport))
			{
				shutdown(sock_server_listen, 2);
				closesocket(connection);
				continue;
			}

			strcpy(dest_host,remotehost);
			dest_port=remoteport;
			shutdown(sock_server_listen, 2);
			local_in = local_out=connection;
			if (strcmp(dest_host,"ID")==0)
			{
				found2=TRUE;
				if (saved_refuse2)
				{
					found2=FALSE;
					for (i=0;i<rule3;i++)
						{
							if (atoi(temp3[i])==remoteport) found2=true;

						}
				}
				if (saved_usecom)
				{
					if (lookup_comment(remoteport)==NULL) found2=FALSE;
				}

				if (!found2) 
					{
					debug("ID refused %i \n",remoteport);
				    closesocket(connection);
					}
				if (found2)
				{
					teststruct.remote=connection;
					teststruct.code=remoteport;
					strcpy_s(teststruct.hostname,ip_peer);
					
					Add_server_list(&teststruct);

					if (Find_viewer_list(&teststruct)!=MAX_LIST+1)
					{
						debug("ID viewer found %i \n",remoteport);
						//initiate connection
						Find_server_list(&teststruct);
						teststruct.local_in=Viewers[Find_viewer_list(&teststruct)].local_in;
						teststruct.local_out=Viewers[Find_viewer_list(&teststruct)].local_out;
						teststruct.remote=connection;
						teststruct.viewer_nummer=Find_viewer_list(&teststruct);
						teststruct.server_nummer=Find_server_list(&teststruct);
						teststruct.server=1;
						CreateThread(NULL,0,do_repeater,(LPVOID)&teststruct,0,&dwThreadId);
					}
					else
						{							
							if (Find_server_list_id(&teststruct)!=MAX_LIST+1)
							{
								teststruct.server=1;
								if (teststruct.waitingThread==0)CreateThread(NULL,0,do_repeater_wait,(LPVOID)&teststruct,0,&dwThreadId);
							}
						}
				}
			}
		}
end:;
   
    }
	notstopped=false;
    debug ("relaying done. server_listen \n");
    closesocket(local_out);

	closesocket(local_in);
    return 0;
}

void Start_server_listenThread()
{
	notstopped=1;
	DWORD iID;
	hserver_listen=CreateThread(NULL, 0, server_listen, NULL, 0, &iID);
}

void Stop_server_listenThread()
{
	closesocket(sock_server_listen);
	notstopped=0;
	if (hserver_listen)
		{
			WaitForSingleObject(hserver_listen,5000);
			CloseHandle(hserver_listen);
			hserver_listen=NULL;
		}
}
