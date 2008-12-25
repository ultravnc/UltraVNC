#include "stdafx.h"
#include "log.h"
#include <stdio.h>

#define LOG_LINES 7
extern HWND EditControl;
extern HWND EditControl2;

static struct LIST {
  struct LIST *next;
  int len;
  char txt[1]; /* single character for trailing '\0' */
} *head=NULL, *tail=NULL;

static char *log_txt(void) {
    char *buff;
    int ptr=0, len=0;
    struct LIST *curr;

 //   enter_critical_section(CRIT_WIN_LOG);
    for(curr=head; curr; curr=curr->next)
        len+=curr->len+2; /* +2 for trailing '\r\n' */
    buff=(char *)malloc(len+1); /* +1 for trailing '\0' */
    for(curr=head; curr; curr=curr->next) {
        memcpy(buff+ptr, curr->txt, curr->len);
        ptr+=curr->len;
        if(curr->next) {
            buff[ptr++]='\r';
            buff[ptr++]='\n';
        }
    }
    buff[ptr]='\0';
 //   leave_critical_section(CRIT_WIN_LOG);

    return buff;
}

void
win_log_cleanup()
{
	struct LIST *curr;
	struct LIST *temp;

	curr=head;
	if (curr==NULL) return;
	temp=curr->next;
	//check
	if(curr!=NULL) free(curr);
	while(temp!=NULL)
	{
		curr=temp;
		temp=curr->next;
		free(curr);
	}

}


void win_log(char *line) { /* Also used in log.c */
    struct LIST *curr;
    int len;
    static int log_len=0;
    char *txt;
    len=strlen(line);
    curr=(LIST *)malloc(sizeof(struct LIST)+len);
    curr->len=len;
    strcpy(curr->txt, line);
    curr->next=NULL;

 //   enter_critical_section(CRIT_WIN_LOG);
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
        txt=log_txt();
        SetWindowText(EditControl, txt);
		SetWindowText(EditControl2, txt);
        free(txt);
}

void
debug( const char *fmt, ... )
{
	char myoutput[256];
	char myoutput2[256];
    va_list args;
	memset(myoutput2,0,256);
	va_start( args, fmt );
	sprintf(myoutput2, "> ");
	vsprintf( myoutput, fmt, args );
	va_end( args );
	strncat(myoutput2,myoutput,strlen(myoutput));
	win_log(myoutput2);
}