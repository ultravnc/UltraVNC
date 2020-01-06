/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002 Ultr@VNC Team Members. All Rights Reserved.
//
//
//  The VNC system is free software; you can redistribute it and/or modify
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
//
/////////////////////////////////////////////////////////////////////////////

#include "repeater.h"

extern int notstopped;

static int visible;


void
LogStats(int code,long recv,long send)
{
char szFileName[MAX_PATH];
char tempchar[128];
HANDLE hFile=NULL;
FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat_s (szFileName,MAX_PATH,"\\");
						_itoa_s(code,tempchar,10);
						strcat_s (szFileName,MAX_PATH, tempchar);
						strcat_s (szFileName,MAX_PATH,".txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "a")) != NULL)
		{
			char	msg[100];
			char	buf[5];
			SYSTEMTIME	st; 
			GetLocalTime(&st);
			_itoa_s(st.wYear,buf,10);
			strcpy_s(msg,buf);
			strcat_s(msg,"/");
			_itoa_s(st.wMonth,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,"/");
			_itoa_s(st.wDay,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg," ");
			_itoa_s(st.wHour,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,":");
			_itoa_s(st.wMinute,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,":");
			_itoa_s(st.wSecond,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg," ");
			strcat_s(msg,"Transmitted: ");
			_ltoa_s((send+recv)/512,tempchar,10);
			strcat_s(msg, tempchar);
			strcat_s(msg,"k \n");
	
			fprintf(f,msg);
			fclose(f);
	}
}
void
LogStats_server(int code,int nummer)
{
char szFileName[MAX_PATH];
//char tempchar[128];
    HANDLE hFile=NULL;
	FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat_s(szFileName,"\\");
						strcat_s(szFileName,"server_access.txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "a")) != NULL)
		{
			char	msg[100];
			char	buf[5];
			SYSTEMTIME	st; 
			GetLocalTime(&st);
			_itoa_s(st.wYear,buf,10);
			strcpy_s(msg,buf);
			strcat_s(msg,"/");
			_itoa_s(st.wMonth,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,"/");
			_itoa_s(st.wDay,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg," ");
			_itoa_s(st.wHour,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,":");
			_itoa_s(st.wMinute,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,":");
			_itoa_s(st.wSecond,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg," ");
	
			fprintf(f,"%s;%i;%s\n",msg,code,Servers[nummer].hostname);
			fclose(f);
	}
}

void
LogStats_viewer(int code,int nummer)
{
char szFileName[MAX_PATH];
//char tempchar[128];
    HANDLE hFile=NULL;
	FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat_s(szFileName,"\\");
						strcat_s(szFileName,"viewer_access.txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "a")) != NULL)
		{
			char	msg[100];
			char	buf[5];
			SYSTEMTIME	st; 
			GetLocalTime(&st);
			_itoa_s(st.wYear,buf,10);
			strcpy_s(msg,buf);
			strcat_s(msg,"/");
			_itoa_s(st.wMonth,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,"/");
			_itoa_s(st.wDay,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg," ");
			_itoa_s(st.wHour,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,":");
			_itoa_s(st.wMinute,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg,":");
			_itoa_s(st.wSecond,buf,10);
			strcat_s(msg,buf);
			strcat_s(msg," ");
	
			fprintf(f,"%s;%i;%s\n",msg,code,Viewers[nummer].hostname);
			fclose(f);
	}
}

void LogStats_access(char *start,char *stop,int code,int viewer,int server ,long bytes)
{
	char szFileName[MAX_PATH];
//	char tempchar[128];
    HANDLE hFile=NULL;
	FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat_s(szFileName,"\\");
						strcat_s(szFileName,"connections.txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "a")) != NULL)
		{
			fprintf(f,"%s;%s;%i;%s;%s;%u\n",start,stop,code,Viewers[viewer].hostname,Servers[server].hostname,bytes);
			fclose(f);
	}
}

void
debug( const char *fmt, ... )
{
	char myoutput[256];
	char myoutput2[256];
    va_list args;
	memset(myoutput2,0,256);
	va_start( args, fmt );
	sprintf_s(myoutput, "UltraVnc> ");
	vsprintf_s( myoutput, fmt, args );
	va_end( args );
	strncpy_s(myoutput2,myoutput,strlen(myoutput)-1);
	win_log(myoutput2);
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
main_test()// int argc, char **argv )
{
	while(notwebstopped)
	{
		notstopped=1;
		WSADATA wsadata;
		WSAStartup( 0x101, &wsadata);
		Clean_server_List();
		Clean_viewer_List();
	

		debug("Copyright (C) 2005 Ultr@VNC Team Members. All Rights Reserved.\n");
		debug("\n");
		debug("\n");
		debug("The Repeater is free software; you can redistribute it and/or modify\n");
		debug("it under the terms of the GNU General Public License as published by\n");
		debug("the Free Software Foundation; either version 2 of the License, or\n");
		debug("(at your option) any later version.\n");
		debug(" \n");
		debug("This program is distributed in the hope that it will be useful,\n");
		debug("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
		debug("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
		debug("GNU General Public License for more details.\n");
		debug(" \n");
		debug("You should have received a copy of the GNU General Public License\n");
		debug("along with this program; if not, write to the Free Software\n");
		debug("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,\n");
		debug("USA.\n");
		debug(" \n");

    
		if (saved_mode2) Start_server_listenThread();
		Start_cleaupthread();
		Start_mode12listenerThread();
		while (notstopped) 
			Sleep(100);
		Stop_server_listenThread();
		Stop_cleaupthread();
		Stop_mode12listenerThread();
		WSACleanup();
	}
    return 0;
}