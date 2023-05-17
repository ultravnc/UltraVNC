/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

#include "uvncUiAccess.h"
#include "vncOSVersion.h"

comm_serv *keyEventFn=NULL;
comm_serv *StopeventFn=NULL;
comm_serv *StarteventFn=NULL;

int g_lockcode = 0;
int g_initialized = false;
int errorcounter =0;
int preventDeadlock = 0;

mini_lock::mini_lock(int lockcode)
{
	preventDeadlock = 0;
	while (g_lockcode != 0) {

		Sleep(100);
		preventDeadlock++;
		if (preventDeadlock > 20)
			g_lockcode = 0;
	}
	g_lockcode = lockcode;
}

mini_lock::~mini_lock()
{
	g_lockcode = 0;
}


bool Shellexecuteforuiaccess()
{		
		char WORKDIR[MAX_PATH];
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
				{
				char* p = strrchr(WORKDIR, '\\');
				if (p == NULL) return false;
				*p = '\0';
				}
		strcat_s(WORKDIR,"\\uvnckeyboardhelper.exe");
	
		FILE *fp = fopen(WORKDIR,"rb");
		if(fp) fclose(fp);
		else  return false;
				
		SHELLEXECUTEINFO shExecInfo;
		memset(&shExecInfo,0,sizeof(shExecInfo));
		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = NULL;
		shExecInfo.hwnd = NULL;
		shExecInfo.lpVerb = "runas";
		shExecInfo.lpFile = WORKDIR;
		shExecInfo.lpParameters ="";
		shExecInfo.lpDirectory = NULL;
		shExecInfo.nShow = SW_HIDE;
		shExecInfo.hInstApp = NULL;
		if (!ShellExecuteEx(&shExecInfo))
		{
			return false;
		}
		return true;
		Sleep(2000);
}

void keepalive()
{
	if (!VNC_OSVersion::getInstance()->OS_WIN8 || errorcounter > 3) 
		return;
	mini_lock ml(1);
	unsigned char Invalue=12;
	unsigned char Outvalue=0;
	if (StarteventFn) {
		StarteventFn->Call_Fnction_Long_Timeout((char*)&Invalue,(char*)&Outvalue,1);
		if (Invalue == Outvalue) {
			errorcounter=0;
			return;
		}
	}
	//Try to reinit the keyboard
	keybd_initialize_no_crit();
	errorcounter++;
	if (errorcounter>3) 
		goto error;
	return;
	// This disable the keyboard helper, better to have something
	error:
	if (keyEventFn)delete keyEventFn;
			keyEventFn=NULL;
	if (StopeventFn)delete StopeventFn;
			StopeventFn=NULL;
	if (StarteventFn)delete StarteventFn;
			StarteventFn=NULL;
}

void keybd_uni_event(_In_  BYTE bVk,_In_  BYTE bScan,_In_  DWORD dwFlags,_In_  ULONG_PTR dwExtraInfo)
{
	if (!VNC_OSVersion::getInstance()->OS_WIN8 || g_lockcode != 0 || !g_initialized) {
		keybd_event(bVk,bScan,dwFlags,dwExtraInfo);
		return;
	}
	mini_lock ml(2);
	bool ldown = HIBYTE(::GetKeyState(VK_LMENU)) != 0;
	bool lcdown = HIBYTE(::GetKeyState(VK_LCONTROL)) != 0;	
	bool lwindown = HIBYTE(::GetKeyState(VK_LWIN)) != 0;
	// The shared memory trick to inject keys seems to generate an overhead when fast key presses are done.
	// Actual we only need it for special keys, so we better ctivate the check
	 if (keyEventFn==NULL || (!ldown && !lcdown && !lwindown) )
		 keybd_event(bVk,bScan,dwFlags,dwExtraInfo);
	 else {
		keyEventdata ked;
		ked.bVk=bVk;
		ked.bScan=bScan;	
		ked.dwflags=dwFlags;
		keyEventFn->Call_Fnction((char*)&ked,NULL);		
	 }
}

void keybd_initialize_no_crit()
{
{
	if (keyEventFn)delete keyEventFn;
		keyEventFn=NULL;
	if (StopeventFn)delete StopeventFn;
		StopeventFn=NULL;
	if (StarteventFn)delete StarteventFn;
		StarteventFn=NULL;
	keyEventFn=new comm_serv;
	StopeventFn=new comm_serv;
	StarteventFn=new comm_serv;
	if (!keyEventFn->Init("keyEvent",sizeof(keyEventdata),0,false,true)) 
		goto error;
	if (!StopeventFn->Init("stop_event",0,0,false,true)) 
		goto error;
	if (!StarteventFn->Init("start_event",1,1,false,true))
		goto error;	
	if (!Shellexecuteforuiaccess())
		goto error;
	unsigned char Invalue=12;
	unsigned char Outvalue=0;
	if (StarteventFn == NULL)
		goto error;
	int StartErrorCounter = 0;
	while (Invalue!=Outvalue) {
		StarteventFn->Call_Fnction_Long_Timeout((char*)&Invalue,(char*)&Outvalue,1);
		StartErrorCounter++;
		if (StartErrorCounter > 5)
			goto error;	
	}
	return;
}

error:
	if (keyEventFn)delete keyEventFn;
			keyEventFn=NULL;
	if (StopeventFn)delete StopeventFn;
			StopeventFn=NULL;
	if (StarteventFn)delete StarteventFn;
			StarteventFn=NULL;
	return;
}

void keybd_initialize()
{
{
	if (!VNC_OSVersion::getInstance()->OS_WIN8) 
		return;
	g_initialized = true;
	errorcounter = 0;
	g_lockcode = 0;
	mini_lock ml(4);
	keyEventFn=new comm_serv;
	StopeventFn=new comm_serv;
	StarteventFn=new comm_serv;
	if (!keyEventFn->Init("keyEvent",sizeof(keyEventdata),0,false,true)) 
		goto error;
	if (!StopeventFn->Init("stop_event",0,0,false,true)) 
		goto error;
	if (!StarteventFn->Init("start_event",1,1,false,true)) 
		goto error;	
	if (!Shellexecuteforuiaccess()) 
		goto error;	
	unsigned char Invalue=12;
	unsigned char Outvalue=0;
	if (StarteventFn == NULL)
		goto error;
	int StartErrorCounter = 0;
	while (Invalue!=Outvalue) {
		StarteventFn->Call_Fnction_Long_Timeout((char*)&Invalue,(char*)&Outvalue, 1);
		StartErrorCounter++;
		if (StartErrorCounter > 5)
			goto error;	
	}
	return;

}
error:
	if (keyEventFn)delete keyEventFn;
			keyEventFn=NULL;
	if (StopeventFn)delete StopeventFn;
			StopeventFn=NULL;
	if (StarteventFn)delete StarteventFn;
			StarteventFn=NULL;
	return;
}

void keybd_delete()
{
	if (!VNC_OSVersion::getInstance()->OS_WIN8) 
		return;
	if (StopeventFn) StopeventFn->Call_Fnction_no_feedback();
	if (keyEventFn)delete keyEventFn;
	keyEventFn=NULL;
	if (StopeventFn)delete StopeventFn;
	StopeventFn=NULL;
	if (StarteventFn)delete StarteventFn;
	StarteventFn=NULL;
	g_initialized=false;
}

comm_serv::comm_serv()
{
	event_E_IN=NULL;
	event_E_IN_DONE=NULL;
	event_E_OUT=NULL;
	event_E_OUT_DONE=NULL;
	data_IN=NULL;
	data_OUT=NULL;
	hMapFile_IN=NULL;
	hMapFile_OUT=NULL;
	InitializeCriticalSection(&CriticalSection_IN); 
	InitializeCriticalSection(&CriticalSection_OUT); 
	timeout_counter=0;
	GLOBAL_RUNNING=true;
}

comm_serv::~comm_serv()
{
	GLOBAL_RUNNING=false;
	if (event_E_IN) CloseHandle(event_E_IN);
	if (event_E_IN_DONE) CloseHandle(event_E_IN_DONE);
	if (event_E_OUT) CloseHandle(event_E_OUT);
	if (event_E_OUT_DONE) CloseHandle(event_E_OUT_DONE);
	if (data_IN)UnmapViewOfFile(data_IN);
	if (data_OUT)UnmapViewOfFile(data_OUT);
	if (hMapFile_IN)CloseHandle(hMapFile_IN);
	if (hMapFile_OUT)CloseHandle(hMapFile_OUT);
	DeleteCriticalSection(&CriticalSection_IN);
	DeleteCriticalSection(&CriticalSection_OUT);
}


void 
	comm_serv::create_sec_attribute()
{
		char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = FALSE;
		secAttr.lpSecurityDescriptor = &secDesc;
		InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);

		PSECURITY_DESCRIPTOR pSD;
		BOOL retcode =ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)",SDDL_REVISION_1,&pSD,NULL);

		if(retcode != 0){ 
		PACL pSacl = NULL;
		BOOL fSaclPresent = FALSE;
		BOOL fSaclDefaulted = FALSE;
		retcode =GetSecurityDescriptorSacl(
			pSD,
			&fSaclPresent,
			&pSacl,
			&fSaclDefaulted);
		if (pSacl) retcode =SetSecurityDescriptorSacl(secAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE); 
		}
}

bool comm_serv::Init(char *name,int IN_datasize_IN,int IN_datasize_OUT,bool app,bool master)
{
	datasize_IN = IN_datasize_IN;
	datasize_OUT = IN_datasize_OUT;

	char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = FALSE;
		secAttr.lpSecurityDescriptor = &secDesc;
		InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);

		PSECURITY_DESCRIPTOR pSD;
		BOOL retcode =ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)",SDDL_REVISION_1,&pSD,NULL);

		if(retcode != 0){ 
		PACL pSacl = NULL;
		BOOL fSaclPresent = FALSE;
		BOOL fSaclDefaulted = FALSE;
		retcode =GetSecurityDescriptorSacl(
			pSD,
			&fSaclPresent,
			&pSacl,
			&fSaclDefaulted);
		if (pSacl) retcode =SetSecurityDescriptorSacl(secAttr.lpSecurityDescriptor, TRUE, pSacl, FALSE); 
		}

	char savename[42];
	strcpy_s(savename,42,name);
	if (app)
	{
		strcpy_s(filemapping_IN,64,"");
		strcpy_s(filemapping_OUT,64,"");
		strcpy_s(event_IN,64,"");
		strcpy_s(event_IN_DONE,64,"");
		strcpy_s(event_OUT,64,"");
		strcpy_s(event_OUT_DONE,64,"");

		strcat_s(filemapping_IN,64,name);
		strcat_s(filemapping_IN,64,"fm_IN");
		strcat_s(filemapping_OUT,64,name);
		strcat_s(filemapping_OUT,64,"fm_OUT");
		strcat_s(event_IN,64,name);
		strcat_s(event_IN,64,"event_IN");
		strcat_s(event_IN_DONE,64,name);
		strcat_s(event_IN_DONE,64,"event_IN_DONE");
		strcat_s(event_OUT,64,name);
		strcat_s(event_OUT,64,"event_OUT");
		strcat_s(event_OUT_DONE,64,name);
		strcat_s(event_OUT_DONE,64,"event_OUT_DONE");
	}
	else
	{
		strcpy_s(filemapping_IN,64,"Global\\");
		strcpy_s(filemapping_OUT,64,"Global\\");
		strcpy_s(event_IN,64,"Global\\");
		strcpy_s(event_IN_DONE,64,"Global\\");
		strcpy_s(event_OUT,64,"Global\\");
		strcpy_s(event_OUT_DONE,64,"Global\\");

		strcat_s(filemapping_IN,64,name);
		strcat_s(filemapping_IN,64,"fm_IN");
		strcat_s(filemapping_OUT,64,name);
		strcat_s(filemapping_OUT,64,"fm_OUT");
		strcat_s(event_IN,64,name);
		strcat_s(event_IN,64,"event_IN");
		strcat_s(event_IN_DONE,64,name);
		strcat_s(event_IN_DONE,64,"event_IN_DONE");
		strcat_s(event_OUT,64,name);
		strcat_s(event_OUT,64,"event_OUT");
		strcat_s(event_OUT_DONE,64,name);
		strcat_s(event_OUT_DONE,64,"event_OUT_DONE");
	}

	if (master)
	{
	if (!app)
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,&secAttr,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return false;
		data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
		if(data_IN==NULL) return false;
		}
		event_E_IN=CreateEvent(&secAttr, FALSE, FALSE, event_IN);
		if(event_E_IN==NULL) return false;
		event_E_IN_DONE=CreateEvent(&secAttr, FALSE, FALSE, event_IN_DONE);
		if(event_E_IN_DONE==NULL) return false;

		if (datasize_OUT!=0)
		{
		hMapFile_OUT = CreateFileMapping(INVALID_HANDLE_VALUE,&secAttr,PAGE_READWRITE,0,datasize_OUT,filemapping_OUT);
		if (hMapFile_OUT == NULL) return false;
		data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
		if(data_OUT==NULL) return false;
		}
		event_E_OUT=CreateEvent(&secAttr, FALSE, FALSE, event_OUT);
		if(event_E_OUT==NULL) return false;
		event_E_OUT_DONE=CreateEvent(&secAttr, FALSE, FALSE, event_OUT_DONE);
		if(event_E_OUT_DONE==NULL) return false;
	}
	else
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return false;
		data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
		if(data_IN==NULL) return false;
		}
		event_E_IN=CreateEvent(NULL, FALSE, FALSE, event_IN);
		if(event_E_IN==NULL) return false;
		event_E_IN_DONE=CreateEvent(NULL, FALSE, FALSE, event_IN_DONE);
		if(event_E_IN_DONE==NULL) return false;

		if (datasize_OUT!=0)
		{
		hMapFile_OUT = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,datasize_OUT,filemapping_OUT);
		if (hMapFile_OUT == NULL) return false;
		data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
		if(data_OUT==NULL) return false;
		}
		event_E_OUT=CreateEvent(NULL, FALSE, FALSE, event_OUT);
		if(event_E_OUT==NULL) return false;
		event_E_OUT_DONE=CreateEvent(NULL, FALSE, FALSE, event_OUT_DONE);
		if(event_E_OUT_DONE==NULL) return false;
	}
	}
	else
	{
		if (!app)
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return false;
			data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
			if(data_IN==NULL) return false;
			}
			event_E_IN=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN);
			if(event_E_IN==NULL) return false;
			ResetEvent(event_E_IN);
			event_E_IN_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN_DONE);
			if(event_E_IN_DONE==NULL) return false;
			ResetEvent(event_E_IN_DONE);
			if (datasize_OUT!=0)
			{
			hMapFile_OUT = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_OUT);
			if (hMapFile_OUT == NULL) return false;
			data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
			if(data_OUT==NULL) return false;
			}
			event_E_OUT=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT);
			if(event_E_OUT==NULL) return false;
			ResetEvent(event_E_OUT);
			event_E_OUT_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT_DONE);
			if(event_E_OUT_DONE==NULL) return false;
			ResetEvent(event_E_OUT_DONE);
		}
		else
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return false;
			data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
			if(data_IN==NULL) return false;
			}
			event_E_IN=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN);
			if(event_E_IN==NULL) return false;
			ResetEvent(event_E_IN);
			event_E_IN_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN_DONE);
			if(event_E_IN_DONE==NULL) return false;
			ResetEvent(event_E_IN_DONE);

			if (datasize_OUT!=0)
			{
			hMapFile_OUT =OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_OUT);
			if (hMapFile_OUT == NULL) return false;
			data_OUT=(char*)MapViewOfFile(hMapFile_OUT,FILE_MAP_ALL_ACCESS,0,0,datasize_OUT);           
			if(data_OUT==NULL) return false;
			}
			event_E_OUT=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT);
			if(event_E_OUT==NULL) return false;
			ResetEvent(event_E_OUT);
			event_E_OUT_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_OUT_DONE);
			if(event_E_OUT_DONE==NULL) return false;
			ResetEvent(event_E_OUT_DONE);
		}
	}
	return true;
}

//service call session function
void comm_serv::Call_Fnction(char *databuffer_IN,char *databuffer_OUT)
{
	if (!GLOBAL_RUNNING) return;
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	//ResetEvent(event_E_IN_DONE);
	if (event_E_OUT) ResetEvent(event_E_OUT);
	if (event_E_OUT_DONE) ResetEvent(event_E_OUT_DONE);
	if (event_E_IN) SetEvent(event_E_IN);
	DWORD r=WaitForSingleObject(event_E_IN_DONE,1000);

	if (r==WAIT_TIMEOUT) 
	{
		r=1;
		timeout_counter++;
	}
	else
	{
		timeout_counter=0;
	}

	if (timeout_counter>3)
	{
		GLOBAL_RUNNING=false;
	}
	if (!GLOBAL_RUNNING) goto error;
	r=WaitForSingleObject(event_E_OUT,1000);
	memcpy(databuffer_OUT,data_OUT,datasize_OUT);
	error:
	if (event_E_OUT_DONE) SetEvent(event_E_OUT_DONE);
	LeaveCriticalSection(&CriticalSection_IN);
}

void comm_serv::Call_Fnction_no_feedback()
{
	SetEvent(event_E_IN);
}

void comm_serv::Call_Fnction_no_feedback_data(char *databuffer_IN,char *databuffer_OUT)
{
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	SetEvent(event_E_IN);
	LeaveCriticalSection(&CriticalSection_IN);
}

//service call session function
void comm_serv::Call_Fnction_Long(char *databuffer_IN,char *databuffer_OUT)
{
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	SetEvent(event_E_IN);
	DWORD r=WaitForSingleObject(event_E_IN_DONE,10000);
	if (r==WAIT_TIMEOUT) 
		r=1;
		
	LeaveCriticalSection(&CriticalSection_IN);

	EnterCriticalSection(&CriticalSection_OUT);
	r=WaitForSingleObject(event_E_OUT,10000);
	if (r==WAIT_TIMEOUT) 
		r=1;

	memcpy(databuffer_OUT,data_OUT,datasize_OUT);
	if (event_E_OUT_DONE) SetEvent(event_E_OUT_DONE);
	LeaveCriticalSection(&CriticalSection_OUT);
}

//service call session function
void comm_serv::Call_Fnction_Long_Timeout(char *databuffer_IN,char *databuffer_OUT,int timeout)
{
	timeout=(timeout+1)*1000;
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	ResetEvent(event_E_IN_DONE);
	ResetEvent(event_E_OUT);
	SetEvent(event_E_IN);
	DWORD r=WaitForSingleObject(event_E_IN_DONE,timeout);		
	LeaveCriticalSection(&CriticalSection_IN);
	if (r==WAIT_TIMEOUT) 
	{
		unsigned char value=99;
		memcpy(databuffer_OUT,&value,datasize_OUT);
		return;
	}

	EnterCriticalSection(&CriticalSection_OUT);
	r=WaitForSingleObject(event_E_OUT,timeout);
	memcpy(databuffer_OUT,data_OUT,datasize_OUT);
	if (event_E_OUT_DONE) SetEvent(event_E_OUT_DONE);
	if (r==WAIT_TIMEOUT) 
	{
		unsigned char value=99;
		memcpy(databuffer_OUT,&value,datasize_OUT);
		LeaveCriticalSection(&CriticalSection_OUT);
		return;
	}
	LeaveCriticalSection(&CriticalSection_OUT);
}

HANDLE comm_serv::GetEvent()
{
	return event_E_IN;
}

char *comm_serv::Getsharedmem()
{
	return data_IN;
}

void comm_serv::ReadData(char *databuffer)
{
	memcpy(databuffer,data_IN,datasize_IN);
	if (event_E_IN_DONE) SetEvent(event_E_IN_DONE);
}

void comm_serv::SetData(char *databuffer)
{
	if (!GLOBAL_RUNNING) return;
	memcpy(data_OUT,databuffer,datasize_OUT);
	if (event_E_OUT)SetEvent(event_E_OUT);
	WaitForSingleObject(event_E_OUT_DONE,2000);	
}

void comm_serv::Force_unblock()
{
	if (event_E_OUT_DONE) SetEvent(event_E_OUT_DONE);
	if (event_E_IN_DONE) SetEvent(event_E_IN_DONE);
	if (event_E_OUT) SetEvent(event_E_OUT);
}

void comm_serv::Release()
{
	if (event_E_IN) ResetEvent(event_E_IN);
}