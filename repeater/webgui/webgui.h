#include <stdio.h>

#define true TRUE
#define false FALSE
#define MAX_LIST 1000

typedef struct _mycomstruct
{
	char comment[256];
	ULONG code;
}mycomstruct,*pmycomstruct;

typedef struct _mystruct
{
	SOCKET local_in;
	SOCKET local_out;
	SOCKET remote;
	ULONG	code;
	DWORD timestamp;
	BOOL used;
	BOOL waitingThread;
	BOOL server;
	int nummer;
	int size_buffer;
	char preloadbuffer[1028];
	char hostname[16];
	char extracheck[64];
	char time[100];
	long recvbytes;
	long sendbytes;
	long average;
	int viewer_nummer;
	int server_nummer;
}mystruct,*pmystruct;

extern mystruct Servers[MAX_LIST];
extern mystruct Viewers[MAX_LIST];
extern mycomstruct comment[MAX_LIST*2];

DWORD WINAPI ThreadStartWeb(LPVOID);
char * lookup_comment(ULONG code);

void Read_settings();
void Save_settings();
void win_log(char *line);

extern int saved_mode2;
extern int saved_mode1;

extern int saved_portA;
extern int saved_portB;

extern int saved_allow;
extern int saved_refuse;
extern int saved_refuse2;
extern char saved_sample1[1024];
extern char saved_sample2[1024];
extern char saved_sample3[1024];

extern char temp1[50][16];
extern char temp2[50][16];
extern char temp3[50][16];
extern int rule1;
extern int rule2;
extern int rule3;

extern int notwebstopped;
extern int notstopped;
extern int saved_usecom;