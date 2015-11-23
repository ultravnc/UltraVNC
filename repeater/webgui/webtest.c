/* webio.c
 *
 * Part of the Webio Open Source lightweight web server.
 *
 * Copyright (c) 2007 by John Bartas
 * All rights reserved.
 *
 * Use license: Modified from standard BSD license.
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising 
 * materials, Web server pages, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by John Bartas. The name "John Bartas" may not be used to 
 * endorse or promote products derived from this software without 
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/* This file is only for testing or making a plain standalone web
 * server app. It is not needed in a web server when the server 
 * is built as a library for inclusion in an application
 * program or an embedded system.
 */

#include <stdio.h>

#include "websys.h"
#include "webio.h"
#include "webfs.h"
#include "wsfdata.h"
#include "webgui.h"
int saved_mode2;
int saved_mode1;

int saved_portA;
int saved_portB;
int saved_portHTTP=80;
int saved_usecom=0;

int saved_allow;
int saved_refuse;
int saved_refuse2;
char saved_sample1[1024];
char saved_sample2[1024];
char saved_sample3[1024];
char saved_password[64]= {"admin"};
void Save_settings();

extern void	exit(int code);
char *log_txt(void);
void
list_viewers(wi_sess * sess,char *txt);
void
list_servers(wi_sess * sess,char *txt);
void
list_connections(wi_sess * sess,char *txt);

mystruct Servers[MAX_LIST];
mystruct Viewers[MAX_LIST];
mycomstruct comment[MAX_LIST*2];

int notstopped=true;
int notwebstopped=true;

void add_comment(ULONG code, char *com);
void Save_comment();

/* Sample authentication code & "database" */
static char test_name[32] = {"admin"};

int   wfs_auth(void * fd, char * name, char * password);
void del_comment(ULONG code);
void Read_comment();

u_long  cticks = 0;
extern socktype wi_listen;

DWORD WINAPI ThreadStartWeb(LPVOID lpParam)
{
	int   sessions = 0;
	while(notwebstopped)
	{
		int old_port=0;
		int error=-1;		
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		sessions = 0;
		wVersionRequested = MAKEWORD( 2, 2 ); 
		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) {
		return 0;
		}
		printf("Webio server starting...\n");



		error = wi_init();
		old_port=saved_portHTTP;
		while (error<0)
		{
			dprintf("wi_init error %d\n", error);
			saved_portHTTP++;
			error = wi_init();
		}

		if (old_port!=saved_portHTTP)
		{
			char text[200];
			sprintf(text,"The defined web port is already in use. \nChanged to http://localhost:%i \nVerify settings!\n Default user and password is admin.",saved_portHTTP);
			MessageBox(NULL,text,"UltraVnc Repeater: Warning",MB_ICONEXCLAMATION);
		}
		/* Install our port-local authentication routine */
		emfs.wfs_fauth = wfs_auth;

		while(notwebstopped)
			{
				sessions = wi_poll();
				if( sessions < 0 )
					{
						// dtrap(); This doesn't do anything on win32 platform.
						goto dtrap;
					}
			}
		dtrap:
		closesocket(wi_listen);
		WSACleanup( );
	 }
   return sessions;
}


/* Return true if user gets access to the embedded file, 0 if not. */

int
wfs_auth(void * fd, char * name, char * password)
{

   /* See if this file requires authentication. */
   EOFILE *    eofile;
   em_file *   emf;

   eofile = (EOFILE *)fd;
   emf = eofile->eo_emfile;

   if(emf->em_flags & EMF_AUTH)
   {
      if(_stricmp(name, test_name))
         return 0;
      if(_stricmp(password, saved_password))
         return 0;
   }
   return 1;
}


void
ws_dtrap(void)
{
   printf("dtrap - need breakpoint");
}

void
panic(char * msg)
{
   printf("panic: %s", msg);
   dtrap();
   //exit(3);
}


int
mode1_ssi(wi_sess * sess, EOFILE * eofile)
{
   if (saved_mode1) wi_printf(sess, "checked=\"checked\"");
   return 0;
}



/* SSI
 *
 * mode2_ssi routine stub
 */

int
mode2_ssi(wi_sess * sess, EOFILE * eofile)
{
	if (saved_mode2) wi_printf(sess, "checked=\"checked\"");
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * sport_ssi routine stub
 */

int
sport_ssi(wi_sess * sess, EOFILE * eofile)
{
   wi_printf(sess, "value=\"%i\"",saved_portA);
   return 0;
}



/* SSI
 *
 * vport_ssi routine stub
 */

int
vport_ssi(wi_sess * sess, EOFILE * eofile)
{
	wi_printf(sess, "value=\"%i\"",saved_portB);
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * acon_ssi routine stub
 */

int
acon_ssi(wi_sess * sess, EOFILE * eofile)
{
	if (saved_allow) wi_printf(sess, "checked=\"checked\"");
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * rcon_ssi routine stub
 */

int
rcon_ssi(wi_sess * sess, EOFILE * eofile)
{
	if (saved_refuse) wi_printf(sess, "checked=\"checked\"");
   /* Add your code here */
   return 0;
}

int
ucom_ssi(wi_sess * sess, EOFILE * eofile)
{
	if (saved_usecom) wi_printf(sess, "checked=\"checked\"");
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * aid_ssi routine stub
 */

int
aid_ssi(wi_sess * sess, EOFILE * eofile)
{
	if (saved_refuse2) wi_printf(sess, "checked=\"checked\"");
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * acons_ssi routine stub
 */

int
acons_ssi(wi_sess * sess, EOFILE * eofile)
{
   wi_printf(sess, "value=\"%s\"",saved_sample1);
   return 0;
}



/* SSI
 *
 * rcons_ssi routine stub
 */

int
rcons_ssi(wi_sess * sess, EOFILE * eofile)
{
  wi_printf(sess, "value=\"%s\"",saved_sample2);
   return 0;
}

int
webport_ssi(wi_sess * sess, EOFILE * eofile)
{
  wi_printf(sess, "value=\"%i\"",saved_portHTTP);
   return 0;
}



/* SSI
 *
 * aids_ssi routine stub
 */

int
aids_ssi(wi_sess * sess, EOFILE * eofile)
{
  wi_printf(sess, "value=\"%s\"",saved_sample3);
   return 0;
}

int 
memory_ssi(wi_sess * sess, EOFILE * eofile)
{
	char txt[4000];
	struct hostent *remoteHost;
	struct in_addr addr;
	char szHostName[128] = "";
	int i=0;
	
	gethostname(szHostName, sizeof(szHostName) - 1);
	remoteHost= gethostbyname(szHostName);


   /* print memory stats to the session's TX buffers */
   wi_printf(sess, "Hostname:  %s <br>", szHostName );

	while (remoteHost->h_addr_list[i] != 0) {
                addr.s_addr = *(u_long *) remoteHost->h_addr_list[i++];
                wi_printf(sess, "IP Address #%d: %s <br>", i, inet_ntoa(addr));
	}
	wi_printf(sess, "Listen Port Viewer: %i<br>", saved_portA);
	wi_printf(sess, "Listen Port Server: %i<br>", saved_portB);
	wi_printf(sess, "Web Server        : %i<br>", saved_portHTTP);
	wi_printf(sess, "Use comment as extra viewer check: %i<br>", saved_usecom);
   wi_printf(sess, "<br>");

   wi_printf(sess, "Connections: <br>");
   list_connections(sess,txt);
   
   wi_printf(sess, "Waiting servers:  <br>");
   list_servers(sess,txt);
   
    wi_printf(sess, "Waiting viewers:  <br>" );
   list_viewers(sess,txt);
   
   return 0;      /* OK return code */
}

int 
log_ssi(wi_sess * sess, EOFILE * eofile)
{
		char *txt;
	txt=log_txt();
	wi_printf(sess, "%s", txt);
	free(txt);
	return 0;
}


int
wi_cvariables(wi_sess * sess, int token)
{
   int   e=0;

   switch(token)
   {
   case MEMHITS_VAR30:
      e = wi_putlong(sess, (u_long)(wi_totalblocks));
      break;
   }
   return e;
}

char * 
testaction2_cgi(wi_sess * sess,  EOFILE * eofile)
{
   char *   id;
   char *   comment;
   id = wi_formvalue(sess, "id");  
   comment = wi_formvalue(sess, "comment"); 
   if (id && comment) 
	   {
		   add_comment(atol(id), comment);
		   Save_comment();
		}
   wi_redirect(sess, "comment.html");

   return( NULL );
}

char * 
testaction3_cgi(wi_sess * sess,  EOFILE * eofile)
{
   char *   id;
   char *   comment;
   int i;
   for (i=0;i<MAX_LIST*2;i++)
	{
	   char idnr[50];
	   char commentnr[50];
	   sprintf (idnr,"%s%i","id",i);
	   sprintf (commentnr,"%s%i","comment",i);
	   id = wi_formvalue(sess, idnr);  
	   comment = wi_formvalue(sess, commentnr); 
	   if (id && comment) 
		   {
			   add_comment(atol(id), comment);			  
			}
   }
    Save_comment();
   wi_redirect(sess, "comment.html");

   return( NULL );
}

//delete
char * 
testaction4_cgi(wi_sess * sess,  EOFILE * eofile)
{
   char *   id;
   id = wi_formvalue(sess, "id");  
   if (id) 
	   {
		   del_comment(atol(id));
		   Save_comment();
		}
   wi_redirect(sess, "comment.html");
   return( NULL );
}

//reload
char * 
testaction5_cgi(wi_sess * sess,  EOFILE * eofile)
{
   Read_comment();
   wi_redirect(sess, "comment.html");
   return( NULL );
}

int 
listcomment_ssi(wi_sess * sess, EOFILE * eofile)
{
	int i;
	char txt[1000];
	for (i=0;i<MAX_LIST*2;i++)
	{
		if (comment[i].code!=0)
		{
			char a[]="<tr>";
			char b[]="<td style=\"width: 20px\">ID: </td>";
			char c[]="<td style=\"width: 72px\">";
			char d[]="<input name=\"id";
			char pd[]="\" type=\"text\" value=\"";
			char dd[]="\"  style=\"width: 72px\"></td>";
			char e[]="<td style=\"width: 72px\">Comment: </td>";
			char f[]="<td style=\"width: 376px\">";
			char g[]="<input name=\"comment";
			char pg[]="\" type=\"text\" value=\"";
			char gg[]="\" style=\"width: 450px\"></td>";
 			char h[]="</tr>";
			if (comment[i].comment) sprintf(txt,"%s%s%s%s%i%s%i%s%s%s%s%i%s%s%s%s",a,b,c,d,i,pd,comment[i].code,dd,e,f,g,i,pg,comment[i].comment,gg,h);
			else sprintf(txt,"%s%s%s%s%i%s%i%s%s%s%s%i%s%s%s%s",a,b,c,d,i,pd,comment[i].code,dd,e,f,g,i,pg,"",gg,h);
			wi_printf(sess, "%s", txt);
		}
	}
	return 0;
}

int 
connections_ssi(wi_sess * sess, EOFILE * eofile)
{	
	char str[1024];
	char szFileName[MAX_PATH];
//	char tempchar[128];
    HANDLE hFile=NULL;
	FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat (szFileName,"\\");
						strcat (szFileName,"connections.txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "r")) != NULL)
	{
		while(! feof(f))
		{
			fgets(str,sizeof(str),f);
			wi_printf(sess, "%s", str);
		}
	fclose(f);
	}
	return 0;
}
int 
server_access_ssi(wi_sess * sess, EOFILE * eofile)
{	
	char str[1024];
	char szFileName[MAX_PATH];
//	char tempchar[128];
    HANDLE hFile=NULL;
	FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat (szFileName,"\\");
						strcat (szFileName,"server_access.txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "r")) != NULL)
	{
	while(! feof(f))
		{
			fgets(str,sizeof(str),f);
			wi_printf(sess, "%s", str);
		}
	fclose(f);
	}
	return 0;
}
int 
viewer_access_ssi(wi_sess * sess, EOFILE * eofile)
{	
	char str[1024];
	char szFileName[MAX_PATH];
//	char tempchar[128];
    HANDLE hFile=NULL;
	FILE *f;
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat (szFileName,"\\");
						strcat (szFileName,"viewer_access.txt");
					}

	if ((f = fopen((LPCSTR)szFileName, "r")) != NULL)
	{
	while(! feof(f))
		{
			fgets(str,sizeof(str),f);
			wi_printf(sess, "%s", str);
		}
	fclose(f);
	}
	return 0;
}

/* testaction_cgi
 * Stub routine for form processing
 * 
 * Returns ????
 */

char username[512];

char * 
testaction_cgi(wi_sess * sess,  EOFILE * eofile)
{
   char *   mode1;
   char *   mode2;
   char *   server_port;
   char *   viewer_port;

    char *   allow_on;
	char *   allow_con;
	char *	 ucom_on;

	char *   refuse_on;
	char *   refuse_con;

	char *   id_on;
	char *   id_con;
	char *	 web_port;



   mode1 = wi_formvalue(sess, "mode1");  
   if (mode1) saved_mode1=true;
   else saved_mode1=false;
   mode2 = wi_formvalue(sess, "mode2");  
    if (mode2) saved_mode2=true;
   else saved_mode2=false;
#ifndef _DEBUG
   server_port = wi_formvalue(sess, "server_port"); 
   if (server_port) saved_portA=atoi(server_port);
   viewer_port = wi_formvalue(sess, "viewer_port");  
   if (viewer_port) saved_portB=atoi(viewer_port);

   allow_on = wi_formvalue(sess, "allow_on"); 
   if (allow_on) saved_allow=true;
   else saved_allow=false;
   allow_con = wi_formvalue(sess, "allow_con");
   if (allow_con) strcpy(saved_sample1,allow_con);
   else strcpy(saved_sample1,"");

   refuse_on = wi_formvalue(sess, "refuse_on");  
   if (refuse_on) saved_refuse=true;
   else saved_refuse=false;
   refuse_con = wi_formvalue(sess, "refuse_con"); 
   if (refuse_con) strcpy(saved_sample2,refuse_con);
   else strcpy(saved_sample2,"");

   id_on = wi_formvalue(sess, "id_on");  
   if (id_on) saved_refuse2=true;
   else saved_refuse2=false;
   id_con = wi_formvalue(sess, "id_con");  
    if (id_con) strcpy(saved_sample3,id_con);
   else strcpy(saved_sample3,"");
#endif
   ucom_on = wi_formvalue(sess, "ucom_on");  
   if (ucom_on) saved_usecom=true;
   else saved_usecom=false;
#ifndef _DEBUG
   web_port = wi_formvalue(sess, "web_port"); 
   if (web_port) saved_portHTTP=atoi(web_port);
#endif
   Save_settings();
   notstopped=0;
   Sleep(2000);
   wi_redirect(sess, "settings.html");

   return( NULL );
}

int VerifyPass(char *passwd)
{
	if (strcmp(saved_password,passwd)==0) return 1;
	else return 0;
}

char * 
passwd_cgi(wi_sess * sess,  EOFILE * eofile)
{
   char *   old_pass;
   char *   new_pass;
    int result=0;

   old_pass = wi_formvalue(sess, "oldpass");   
   new_pass = wi_formvalue(sess, "newpass");   
 
   if (old_pass!=NULL && new_pass!=NULL)
   {
	   result=VerifyPass(old_pass);
	   if (result==1)
	   {
#ifndef _DEBUG
		   strcpy_s(saved_password,64,new_pass);
		   Save_settings();
#endif
		   wi_redirect(sess, "ok.html");
	   }
	   else
	   {
		   wi_redirect(sess, "nok.html");
	   }
   }
   else wi_redirect(sess, "passwd.html");

   return( NULL );
}

/* getname_ssi()
 *
 * SSI routine stub
 */

int
getname_ssi(wi_sess * sess, EOFILE * eofile)
{
    /* output saved user name to session, if it's set */
    if(username[0] == 0)
        wi_printf(sess, "(no user name)");
    else
        wi_printf(sess, "%s", username);
   
    return 0;
}

/* PUSH
 *
 * pushtest_func routine stub
 */

int
pushtest_func(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}

