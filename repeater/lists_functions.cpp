#include "repeater.h"

void
Clean_server_List()
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		Servers[i].code=0;
		Servers[i].used=false;
		Servers[i].waitingThread=false;
	}
}

void
Add_server_list(mystruct *Serverstruct)
{
	char	msg[100];
	char	buf[5];
	SYSTEMTIME	st; 
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Servers[i].code == Serverstruct->code)
		{
			if (strcmp(Servers[i].hostname, Serverstruct->hostname) == 0)  return;
			Servers[i].code = 999999999;
			goto mylabel;
		}
	}
mylabel:
	for (i = 0; i<MAX_LIST; i++)
	{
		if (Servers[i].code ==0)
		{
			debug("Server added to list %i\n", Serverstruct->code);
			Servers[i].code = Serverstruct->code;
			Servers[i].local_in = Serverstruct->local_in;
			Servers[i].local_out = Serverstruct->local_out;
			Servers[i].remote = Serverstruct->remote;
			Servers[i].timestamp = GetTickCount();
			Servers[i].used = false;
			Servers[i].waitingThread = false;
			Servers[i].nummer = i;
			strcpy_s(Servers[i].hostname, Serverstruct->hostname);
			GetLocalTime(&st);
			_itoa(st.wYear, buf, 10);
			strcpy(msg, buf);
			strcat(msg, "/");
			_itoa(st.wMonth, buf, 10);
			strcat(msg, buf);
			strcat(msg, "/");
			_itoa(st.wDay, buf, 10);
			strcat(msg, buf);
			strcat(msg, " ");
			_itoa(st.wHour, buf, 10);
			strcat(msg, buf);
			strcat(msg, ":");
			_itoa(st.wMinute, buf, 10);
			strcat(msg, buf);
			strcat(msg, ":");
			_itoa(st.wSecond, buf, 10);
			strcat(msg, buf);
			strcpy_s(Servers[i].time, msg);
			return;
		}
	}
	return;
}

void 
Remove_server_list(ULONG code)
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Servers[i].code==code)
		{
			debug("Server Removed from list %i\n",code);
			Servers[i].code=0;
			Servers[i].used=false;
			Servers[i].waitingThread=false;
			return;
		}
	}
}

ULONG 
Find_server_list(mystruct *Serverstruct)
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Servers[i].code==Serverstruct->code)
		{
			Servers[i].used=true;
			Serverstruct->nummer=Servers[i].nummer;
			if (Servers[i].waitingThread==1) Sleep(1000);
			return i;
		}
	}
	return MAX_LIST+1;
}
ULONG 
Find_server_list_id(mystruct *Serverstruct)
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Servers[i].code==Serverstruct->code)
		{
			Serverstruct->waitingThread=Servers[i].waitingThread;
			Serverstruct->nummer=Servers[i].nummer;
			return i;
		}
	}
	return MAX_LIST+1;
}

void
Clean_viewer_List()
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		Viewers[i].code=0;
		Viewers[i].used=false;
		Viewers[i].waitingThread=false;
	}
}

void
Add_viewer_list(mystruct *Viewerstruct)
{
	char	msg[100];
	char	buf[5];
	SYSTEMTIME	st; 
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Viewers[i].code==Viewerstruct->code) return;
	}
	for (i=0;i<MAX_LIST;i++)
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
			Viewers[i].waitingThread=false;
			Viewers[i].nummer=i;
			strcpy_s(Viewers[i].hostname,Viewerstruct->hostname);
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
			strcpy_s(Viewers[i].time,msg);
			return;
		}
	}

}

void 
Remove_viewer_list(ULONG code)
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Viewers[i].code==code)
		{
			debug("Viewer removed from list %i\n",code);
			Viewers[i].code=0;
			Viewers[i].used=false;
			Viewers[i].waitingThread=false;
			return;
		}
	}
}

ULONG 
Find_viewer_list(mystruct *Viewerstruct)
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Viewers[i].code==Viewerstruct->code)
		{
			Viewers[i].used=true;
			Viewerstruct->nummer=Viewers[i].nummer;
			if (Viewers[i].waitingThread==1) Sleep(1000);
			return i;
		}
	}
	return MAX_LIST+1;
}

ULONG 
Find_viewer_list_id(mystruct *Viewerstruct)
{
	int i;
	for (i=0;i<MAX_LIST;i++)
	{
		if (Viewers[i].code==Viewerstruct->code)
		{
			Viewerstruct->waitingThread=Viewers[i].waitingThread;
			Viewerstruct->nummer=Viewers[i].nummer;
			return i;
		}
	}
	return MAX_LIST+1;
}



HANDLE hcleanupthread=NULL;

DWORD WINAPI cleaupthread(LPVOID lpParam)
{
	while(notstopped)
	{
		int counterfree=0;
		for ( int ii=0;ii<MAX_LIST;ii++)
			{	
				if (Servers[ii].code==0) counterfree++;
			}
		//if (counterfree<100)
		{
		int i;
		DWORD tick=GetTickCount();
		for (i=0;i<MAX_LIST;i++)
			{
				if (tick-Viewers[i].timestamp>36000000 && Viewers[i].used==false && Viewers[i].code!=0)
				{
					//
					shutdown(Viewers[i].local_in, 1);
					closesocket(Viewers[i].local_in);
					debug("Remove viewer %i %i \n",Viewers[i].code,i);
					Remove_viewer_list(Viewers[i].code);
				}
				if (tick-Servers[i].timestamp>36000000 && Servers[i].used==false && Servers[i].code!=0)
				{
					//
					shutdown(Servers[i].remote,1);
					closesocket(Servers[i].remote);
					debug("Remove server %i\n",Servers[i].code);
					Remove_server_list(Servers[i].code);
				}
			}
		}
//		debug("Searching old connections\n");
		int count=0;
		while(notstopped) 
			{
				Sleep(100);
				count++;
				if (count>6000) break;
			}

	}
	return 0;
}

void Start_cleaupthread()
{
	notstopped=1;
	DWORD iID;
	hcleanupthread=CreateThread(NULL, 0, cleaupthread, NULL, 0, &iID);
}

void Stop_cleaupthread()
{
	notstopped=0;
	debug ("relaying done.\n");

	WaitForSingleObject(hcleanupthread,5000);
	CloseHandle(hcleanupthread);
}