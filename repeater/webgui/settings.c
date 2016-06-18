#include <stdio.h>
#include <setjmp.h>
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <string.h>
#include "websys.h"
#include "webio.h"
#include "webfs.h"
#include "wsfdata.h"
#include "webgui.h"

extern int saved_mode2;
extern int saved_mode1;
extern int saved_keepalive;

extern int saved_portA;
extern int saved_portB;
extern int saved_portHTTP;
extern int saved_usecom;

extern int saved_allow;
extern int saved_refuse;
extern int saved_refuse2;
extern char saved_sample1[1024];
extern char saved_sample2[1024];
extern char saved_sample3[1024];
extern char temp1[50][25];
extern char temp2[50][16];
extern char temp3[50][16];
extern int rule1;
extern int rule2;
extern int rule3;
extern char saved_password[64];
CRITICAL_SECTION cs;

void Save_settings();
char myInifile[MAX_PATH];

void
WriteString(char *key1, char *key2,char *value)
{
	WritePrivateProfileString(key1,key2, value,myInifile);
}
void 
ReadString(char *key1, char *key2,char *value,int valuesize)
{
	GetPrivateProfileString(key1,key2, "",value,valuesize,myInifile);
}

void Read_comment()
{
    int i;
	if (GetModuleFileName(NULL, myInifile, MAX_PATH))
					{
						char* p = strrchr(myInifile, '\\');
						*p = '\0';
						strcat (myInifile,"\\comment.txt");
					}
	for (i=0;i<MAX_LIST*2;i++)
	{
		
		char temp[10];
		char temp2[256];
		char seps[]   = ":";
		char *token;

		comment[i].code=0;
		memset(comment[i].comment,0,256);

		memset(temp2,0,256);
		sprintf(temp,"%i",i);
		ReadString("comment", temp,temp2,256);
		token = strtok( temp2, seps );
		if ( token != NULL )
		{
			int a=0;
			comment[i].code=atol(token);
		}
		token = strtok( NULL, seps );
		if ( token != NULL )
		{
			strcpy_s(comment[i].comment,256,token);
		}
	}
}

void Save_comment()
{
    int i;
	if (GetModuleFileName(NULL, myInifile, MAX_PATH))
					{
						char* p = strrchr(myInifile, '\\');
						*p = '\0';
						strcat (myInifile,"\\comment.txt");
					}
	for (i=0;i<MAX_LIST*2;i++)
	{
		char temp[10];
		char temp3[10];
		char temp2[256];
		sprintf(temp,"%i",i);
		sprintf(temp3,"%u",comment[i].code);
		strcpy_s(temp2,256,temp3);
		strcat_s(temp2,256,":");
		strcat_s(temp2,256,comment[i].comment);
		WriteString("comment", temp,temp2);
	}
}

char * lookup_comment(ULONG code)
{
	int i;
	for (i=0;i<MAX_LIST*2;i++)
	{
		if (comment[i].code==code) return comment[i].comment;
	}
	return NULL;
}

void add_comment(ULONG code, char *com)
{
	int i;
	for (i=0;i<MAX_LIST*2;i++)
	{
		if (comment[i].code==code)
		{
			strcpy_s(comment[i].comment,256,com);
			return;
		}
	}
	for (i=0;i<MAX_LIST*2;i++)
	{
		if (comment[i].code==0)
		{
			strcpy_s(comment[i].comment,256,com);
			comment[i].code=code;
			return;
		}
	}
}

void del_comment(ULONG code)
{
	int i;
	for (i=0;i<MAX_LIST*2;i++)
	{
		if (comment[i].code==code)
		{
			comment[i].code=0;
			strcpy_s(comment[i].comment,256,"");
			return;
		}
	}
}

void
Read_settings()
{
	char szFileName[MAX_PATH];
    HANDLE hFile=NULL;
	Read_comment();
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat (szFileName,"\\settings2.txt");
					}
	hFile=CreateFile((LPCSTR)szFileName, GENERIC_READ,
            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
            NULL);
	if (hFile==INVALID_HANDLE_VALUE)
	{
		saved_portA=5901;
		saved_portB=5500;
		saved_mode2=1;
		saved_mode1=0;
		saved_keepalive=0;
		saved_allow=1;
		saved_refuse=0;
		saved_refuse2=0;
		strcpy(saved_sample1,"");
		strcpy(saved_sample2,"");
		strcpy(saved_sample3,"");
		strcpy(saved_password,"adminadmi2");
		saved_portHTTP=80;
		saved_usecom=0;
		Save_settings();
	}
	else
	{
		unsigned long readbytes;
		char *test;
		char *pos;
		int  ch = ';';
		
		ReadFile(hFile,&saved_portA,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_portB,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_mode2,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_mode1,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_keepalive,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_allow,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_refuse,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_refuse2,sizeof(int),&readbytes,NULL);

		ReadFile(hFile,saved_sample1,1024,&readbytes,NULL);
		ReadFile(hFile,saved_sample2,1024,&readbytes,NULL);
		ReadFile(hFile,saved_sample3,1024,&readbytes,NULL);
		ReadFile(hFile,saved_password,64,&readbytes,NULL);
		ReadFile(hFile,&saved_portHTTP,sizeof(int),&readbytes,NULL);
		ReadFile(hFile,&saved_usecom,sizeof(int),&readbytes,NULL);
		
		test=strchr(saved_sample1,ch);
		pos=saved_sample1;
		rule1=0;
		while (test)
		{
		int len=test-pos;
		strncpy(temp1[rule1],pos,len);
		temp1[rule1][len]=0;
		rule1++;
		pos+=len+1;
		test=strchr(pos,ch);
		}
		strcpy(temp1[rule1],pos);
		rule1++;

		test=strchr(saved_sample2,ch);
		pos=saved_sample2;
		while (test)
		{
		int len=test-pos;
		strncpy(temp2[rule2],pos,len);
		temp2[rule2][len]=0;
		rule2++;
		pos+=len+1;
		test=strchr(pos,ch);
		}
		strcpy(temp2[rule2],pos);
		rule2++;

		test=strchr(saved_sample3,ch);
		pos=saved_sample3;
		while (test)
		{
		int len=test-pos;
		strncpy(temp3[rule3],pos,len);
		temp3[rule3][len]=0;
		rule3++;
		pos+=len+1;
		test=strchr(pos,ch);
		}
		strcpy(temp3[rule3],pos);
		rule3++;

		CloseHandle(hFile);
	}
}

void
Save_settings()
{
	char szFileName[MAX_PATH];
    HANDLE hFile=NULL;
	Save_comment();
	if (GetModuleFileName(NULL, szFileName, MAX_PATH))
					{
						char* p = strrchr(szFileName, '\\');
						*p = '\0';
						strcat (szFileName,"\\settings2.txt");
					}
	hFile=CreateFile((LPCSTR)szFileName, GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
            NULL);
	if (hFile==INVALID_HANDLE_VALUE)
	{
		return ;

	}
	else
	{
		unsigned long readbytes;
		WriteFile(hFile,&saved_portA,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_portB,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_mode2,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_mode1,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_keepalive,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_allow,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_refuse,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_refuse2,sizeof(int),&readbytes,NULL);

		WriteFile(hFile,saved_sample1,1024,&readbytes,NULL);
		WriteFile(hFile,saved_sample2,1024,&readbytes,NULL);
		WriteFile(hFile,saved_sample3,1024,&readbytes,NULL);
		WriteFile(hFile,saved_password,64,&readbytes,NULL);
		WriteFile(hFile,&saved_portHTTP,sizeof(int),&readbytes,NULL);
		WriteFile(hFile,&saved_usecom,sizeof(int),&readbytes,NULL);
		CloseHandle(hFile);


	}
	Read_settings();
}

#define LOG_LINES 20
static struct LIST {
  struct LIST *next;
  int len;
  char txt[1]; 
} *head=NULL, *tail=NULL;

void win_log(char *line) { 	
    struct LIST *curr=NULL;
    int len;
    static int log_len=0;
	EnterCriticalSection(&cs);
    len=strlen(line);
    curr=(struct LIST *)malloc(sizeof(struct LIST)+len);
    curr->len=len;
    strcpy(curr->txt, line);
    curr->next=NULL;

    if(tail)
        tail->next=curr;
    tail=curr;
    if(!head)
        head=tail;
    log_len++;
    while(log_len>LOG_LINES) {
        curr=head;
        head=head->next;
        free(curr);
        log_len--;
    }
	LeaveCriticalSection(&cs);

}

char *log_txt(void) {	
    char *buff=NULL;
    int ptr=0, len=0;
    struct LIST *curr;
	EnterCriticalSection(&cs);

    for(curr=head; curr; curr=curr->next)
        len+=curr->len+4; 
    buff=(char *)malloc(len+1); 
    for(curr=head; curr; curr=curr->next) {
        memcpy(buff+ptr, curr->txt, curr->len);
        ptr+=curr->len;
        if(curr->next) {
            buff[ptr++]='<';
            buff[ptr++]='b';
			buff[ptr++]='r';
			buff[ptr++]='>';
        }
    }
    buff[ptr]='\0';
	LeaveCriticalSection(&cs);
    return buff;
}


void
list_servers(wi_sess * sess,char *txt)
{
	char temp[10];
	int i;
	strcpy(txt,"<table class=\"style2\" style=\"width: 800px\">");
	strcat_s(txt,4000,"<tr>");
		strcat_s(txt,4000,"<td style=\"width: 40px; height: 23px\" class=\"style3\">Slot</td>");
		strcat_s(txt,4000,"<td style=\"width: 160px; height: 23px\" class=\"style3\">Time</td>");
		strcat_s(txt,4000,"<td style=\"width: 90px; height: 23px\" class=\"style3\">ID nummer</td>");
		strcat_s(txt,4000,"<td style=\"width: 150px; height: 23px\" class=\"style3\">Connection from</td>");
		strcat_s(txt,4000,"<td style=\"width: 250px; height: 23px\" class=\"style3\">comment</td>");
	strcat_s(txt,4000,"</tr>");
	wi_printf(sess, "%s", txt );

	for (i=0;i<MAX_LIST;i++)
	{
		if (Servers[i].code!=0 && Servers[i].used==0) 
		{
			strcpy_s(txt,4000,"<tr>");
			_itoa(i,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 40px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"<td style=\"width: 160px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,Servers[i].time);	
			strcat_s(txt,4000,"</td>");

			_itoa(Servers[i].code,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 90px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");
			strcat_s(txt,4000,"<td style=\"width: 150px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,Servers[i].hostname);
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"<td style=\"width: 250px; height: 23px\"class=\"style3\">");
			if (lookup_comment(Servers[i].code)!=NULL) strcat_s(txt,4000,lookup_comment(Servers[i].code));	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"</tr>");
			wi_printf(sess, "%s", txt );
		}
	}
	strcpy(txt,"</table>");
	wi_printf(sess, "%s", txt );

}

void
list_viewers(wi_sess * sess,char *txt)
{
	char temp[10];
	int i;
	strcpy(txt,"<table class=\"style2\" style=\"width: 800px\">");
	strcat_s(txt,4000,"<tr>");
		strcat_s(txt,4000,"<td style=\"width: 40px; height: 23px\" class=\"style3\">Slot</td>");
		strcat_s(txt,4000,"<td style=\"width: 160px; height: 23px\" class=\"style3\">Time</td>");
		strcat_s(txt,4000,"<td style=\"width: 90px; height: 23px\" class=\"style3\">ID nummer</td>");
		strcat_s(txt,4000,"<td style=\"width: 150px; height: 23px\" class=\"style3\">Connection from</td>");
		strcat_s(txt,4000,"<td style=\"width: 250px; height: 23px\" class=\"style3\">comment</td>");
	strcat_s(txt,4000,"</tr>");
	wi_printf(sess, "%s", txt );

	for (i=0;i<MAX_LIST;i++)
	{
		if (Viewers[i].code!=0 && Viewers[i].used==0) 
		{
			strcpy_s(txt,4000,"<tr>");
			_itoa(i,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 40px; height: 23px\" class=\"style3\" >");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"<td style=\"width: 160px; height: 23px\" class=\"style3\" >");
			strcat_s(txt,4000,Viewers[i].time);	
			strcat_s(txt,4000,"</td>");

			_itoa(Viewers[i].code,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 90px; height: 23px\" class=\"style3\" >");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");
			strcat_s(txt,4000,"<td style=\"width: 150px; height: 23px\" class=\"style3\" >");
			strcat_s(txt,4000,Viewers[i].hostname);
			strcat_s(txt,4000,"</td>");
			
			strcat_s(txt,4000,"<td style=\"width: 250px; height: 23px\"class=\"style3\">");
			if ( lookup_comment(Viewers[i].code)!=NULL) strcat_s(txt,4000,lookup_comment(Viewers[i].code));	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"</tr>");
			wi_printf(sess, "%s", txt );
		}
	}
	strcpy(txt,"</table>");
	wi_printf(sess, "%s", txt );

}

void
list_connections(wi_sess * sess,char *txt)
{
	char temp[10];
	int i;
	int j;
	strcpy(txt,"<table class=\"style2\" style=\"width: 800px\">");
	strcat_s(txt,4000,"<tr>");
		strcat_s(txt,4000,"<td style=\"width: 40px; height: 23px\" class=\"style3\">Slot</td>");
		strcat_s(txt,4000,"<td style=\"width: 160px; height: 23px\" class=\"style3\">Time</td>");
		strcat_s(txt,4000,"<td style=\"width: 90px; height: 23px\" class=\"style3\">ID nummer</td>");
		strcat_s(txt,4000,"<td style=\"width: 200px; height: 23px\" class=\"style3\">Viewer</td>");
		strcat_s(txt,4000,"<td style=\"width: 200px; height: 23px\" class=\"style3\">Server</td>");
		strcat_s(txt,4000,"<td style=\"width: 75px; height: 23px\" class=\"style3\">Total kb</td>");
		strcat_s(txt,4000,"<td style=\"width: 75px; height: 23px\" class=\"style3\">kb/s</td>");
		strcat_s(txt,4000,"<td style=\"width: 250px; height: 23px\" class=\"style3\">comment</td>");
	strcat_s(txt,4000,"</tr>");
	wi_printf(sess, "%s", txt );
	strcpy(txt,"");
	for (i=0;i<MAX_LIST;i++)
	{
		if (Viewers[i].code!=0 && Viewers[i].used!=0) 
		{
			strcpy_s(txt,4000,"<tr>");
			_itoa(i,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 40px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"<td style=\"width: 160px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,Viewers[i].time);	
			strcat_s(txt,4000,"</td>");

			_itoa(Viewers[i].code,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 90px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"<td style=\"width: 200px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,Viewers[i].hostname);
			strcat_s(txt,4000,"</td>");

			for (j=0;j<MAX_LIST;j++)
			{
				if (Servers[j].code==Viewers[i].code)
				{
					strcat_s(txt,4000,"<td style=\"width: 200px; height: 23px\"class=\"style3\">");
					strcat_s(txt,4000,Servers[j].hostname);
					strcat_s(txt,4000,"</td>");
				}
			}

			_itoa(Viewers[i].recvbytes/1000,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 75px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");

			_itoa(Viewers[i].average,temp,10);
			strcat_s(txt,4000,"<td style=\"width: 75px; height: 23px\"class=\"style3\">");
			strcat_s(txt,4000,temp);	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"<td style=\"width: 250px; height: 23px\"class=\"style3\">");
			if ( lookup_comment(Viewers[i].code)!=NULL) strcat_s(txt,4000,lookup_comment(Viewers[i].code));	
			strcat_s(txt,4000,"</td>");

			strcat_s(txt,4000,"</tr>");
			wi_printf(sess, "%s", txt );
			strcpy(txt,"");
		}
	}
	strcpy(txt,"</table>");
	wi_printf(sess, "%s", txt );
	strcpy(txt,"");
}