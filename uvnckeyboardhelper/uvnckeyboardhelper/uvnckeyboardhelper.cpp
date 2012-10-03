// uvnckeyboardhelper.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "uvnckeyboardhelper.h"


bool GLOBAL_RUNNING=true;
bool fRunningFromExternalService=false;
comm_serv::comm_serv()
{
	Force_unblock();
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
}

comm_serv::~comm_serv()
{
	CloseHandle(event_E_IN);
	CloseHandle(event_E_IN_DONE);
	CloseHandle(event_E_OUT);
	CloseHandle(event_E_OUT_DONE);
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
		TCHAR * szSD = TEXT("D:")       // Discretionary ACL
			//TEXT("(D;OICI;GA;;;BG)")     // Deny access to built-in guests
			//TEXT("(D;OICI;GA;;;AN)")     // Deny access to anonymous logon
			TEXT("(A;OICI;GRGWGX;;;AU)") // Allow read/write/execute to authenticated users
			TEXT("(A;OICI;GA;;;BA)");    // Allow full control to administrators

		PSECURITY_DESCRIPTOR pSD;
		BOOL retcode =ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)",SDDL_REVISION_1,&pSD,NULL);
		DWORD aa=GetLastError();

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
	datasize_IN=IN_datasize_IN;
	datasize_OUT=IN_datasize_OUT;

	char secDesc[ SECURITY_DESCRIPTOR_MIN_LENGTH ];
		secAttr.nLength = sizeof(secAttr);
		secAttr.bInheritHandle = FALSE;
		secAttr.lpSecurityDescriptor = &secDesc;
		InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);
		TCHAR * szSD = TEXT("D:")       // Discretionary ACL
			//TEXT("(D;OICI;GA;;;BG)")     // Deny access to built-in guests
			//TEXT("(D;OICI;GA;;;AN)")     // Deny access to anonymous logon
			TEXT("(A;OICI;GRGWGX;;;AU)") // Allow read/write/execute to authenticated users
			TEXT("(A;OICI;GA;;;BA)");    // Allow full control to administrators

		PSECURITY_DESCRIPTOR pSD;
		BOOL retcode =ConvertStringSecurityDescriptorToSecurityDescriptor("S:(ML;;NW;;;LW)",SDDL_REVISION_1,&pSD,NULL);
		DWORD aa=GetLastError();

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
		if(event_IN_DONE==NULL) return false;

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
		if(event_OUT_DONE==NULL) return false;
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
		if(event_IN_DONE==NULL) return false;

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
		if(event_OUT_DONE==NULL) return false;
	}
	}
	else
	{
		if (!app)
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			DWORD aa=GetLastError();
			if (hMapFile_IN == NULL) return false;
			data_IN=(char*)MapViewOfFile(hMapFile_IN,FILE_MAP_ALL_ACCESS,0,0,datasize_IN);           
			if(data_IN==NULL) return false;
			}
			event_E_IN=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN);
			if(event_E_IN==NULL) return false;
			ResetEvent(event_E_IN);
			event_E_IN_DONE=OpenEvent(EVENT_ALL_ACCESS, FALSE, event_IN_DONE);
			if(event_IN_DONE==NULL) return false;
			ResetEvent(event_IN_DONE);
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
			if(event_OUT_DONE==NULL) return false;
			ResetEvent(event_OUT_DONE);
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
			if(event_IN_DONE==NULL) return false;
			ResetEvent(event_IN_DONE);

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
			if(event_OUT_DONE==NULL) return false;
			ResetEvent(event_OUT_DONE);
		}
	}
	return true;
}


HANDLE comm_serv::InitFileHandle(char *name,int IN_datasize_IN,int IN_datasize_OUT,bool app,bool master)
{
	datasize_IN=IN_datasize_IN;
	datasize_OUT=IN_datasize_OUT;
	create_sec_attribute();
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
		if (hMapFile_IN == NULL) return NULL;
		}
	}
	else
	{
		if (datasize_IN!=0)
		{
		hMapFile_IN = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,datasize_IN,filemapping_IN);
		if (hMapFile_IN == NULL) return NULL;
		}
	}
	}
	else
	{
		if (!app)
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return NULL;
			}
		}
		else
		{
			if (datasize_IN!=0)
			{
			hMapFile_IN = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,filemapping_IN);
			if (hMapFile_IN == NULL) return NULL;
			}
		}
	}
	return hMapFile_IN;
}

//service call session function
void comm_serv::Call_Fnction(char *databuffer_IN,char *databuffer_OUT)
{
	if (!GLOBAL_RUNNING) return;
	EnterCriticalSection(&CriticalSection_IN);
	memcpy(data_IN,databuffer_IN,datasize_IN);
	//ResetEvent(event_E_IN_DONE);
	ResetEvent(event_E_OUT);
	ResetEvent(event_E_OUT_DONE);
	SetEvent(event_E_IN);
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
	SetEvent(event_E_OUT_DONE);
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
	SetEvent(event_E_OUT_DONE);
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
	SetEvent(event_E_OUT_DONE);
	if (r==WAIT_TIMEOUT) 
	{
		unsigned char value=99;
		memcpy(databuffer_OUT,&value,datasize_OUT);
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
	SetEvent(event_E_IN_DONE);
}

void comm_serv::SetData(char *databuffer)
{
	if (!GLOBAL_RUNNING) return;
	memcpy(data_OUT,databuffer,datasize_OUT);
	SetEvent(event_E_OUT);
	DWORD r=WaitForSingleObject(event_E_OUT_DONE,2000);	
}

void comm_serv::Force_unblock()
{
	SetEvent(event_E_OUT_DONE);
	SetEvent(event_E_IN_DONE);
	SetEvent(event_E_OUT);
}

void comm_serv::Release()
{
	ResetEvent(event_E_IN);
}

bool SelectDesktop()
{
		HDESK desktop;
		HDESK old_desktop;
		DWORD dummy;
		char new_name[256];
		desktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		if (desktop == NULL) return FALSE;
		old_desktop = GetThreadDesktop(GetCurrentThreadId());
		if (!GetUserObjectInformation(desktop, UOI_NAME, &new_name, 256, &dummy)) {
			CloseDesktop(desktop);
			return FALSE;
		}
		if(!SetThreadDesktop(desktop)) {
			CloseDesktop(desktop);
			return FALSE;
		}
		CloseDesktop(old_desktop);			
		return TRUE;

}

bool InputDesktopSelected()
{

	DWORD dummy;
	char threadname[256];
	char inputname[256];
	HDESK threaddesktop = GetThreadDesktop(GetCurrentThreadId());
	HDESK inputdesktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);

	if (inputdesktop == NULL) return FALSE;
	if (!GetUserObjectInformation(threaddesktop, UOI_NAME, &threadname, 256, &dummy)) {
			CloseDesktop(inputdesktop);
			return FALSE;
		}
	if (!GetUserObjectInformation(inputdesktop, UOI_NAME, &inputname, 256, &dummy)) {
			CloseDesktop(inputdesktop);
			return FALSE;
		}	
	CloseDesktop(inputdesktop);
	
	if (strcmp(threadname, inputname) != 0)
		{
			//if (strcmp(inputname, "Screen-saver") == 0)
			{
				return SelectDesktop();
			}
			return FALSE;
		}
	return TRUE;
}

DWORD WINAPI Cadthread(LPVOID lpParam)
{
	HDESK inputdesktop = OpenInputDesktop(0, FALSE,
				DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW |
				DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
				DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS |
				DESKTOP_SWITCHDESKTOP | GENERIC_WRITE);
		SetThreadDesktop(inputdesktop);
		DWORD yerror= GetLastError();

		comm_serv keyEventFn;
		comm_serv StopeventFn;
		comm_serv StarteventFn;
		if (!keyEventFn.Init("keyEvent",sizeof(keyEventdata),0,false,false)) goto error;
		if (!StopeventFn.Init("stop_event",0,0,false,false)) goto error;
		if (!StarteventFn.Init("start_event",1,1,false,false)) goto error;
		HANDLE Events_ini[1];
		Events_ini[0]=StarteventFn.GetEvent();
		DWORD dwEvent = WaitForMultipleObjects(1,Events_ini,FALSE,1000);
		switch(dwEvent)
				{
					case WAIT_OBJECT_0 + 0:
						unsigned char in;
						StarteventFn.ReadData((char*)&in);
						StarteventFn.SetData((char*) &in);
						break;
					case WAIT_TIMEOUT:
						 goto error;
						 break;
				}

		HANDLE Events[3];
		Events[0]=keyEventFn.GetEvent();
		Events[1]=StopeventFn.GetEvent();
		Events[2]=StarteventFn.GetEvent();
		int counter=0;
		while (true)
		{
		DWORD dwEvent = WaitForMultipleObjects(3,Events,FALSE,1000);
		switch(dwEvent)
				{
					case WAIT_OBJECT_0 + 0: 
						Beep(3000,100);
						keyEventdata ked;
						keyEventFn.ReadData((char*)&ked);
						keybd_event(ked.bVk,    ked.bScan,ked.dwflags,0);
						keyEventFn.SetData(NULL);
						break;
					case WAIT_OBJECT_0 + 1: 
						goto error;
						break;
					case WAIT_OBJECT_0 + 2: 						
						unsigned char in;
						StarteventFn.ReadData((char*)&in);
						StarteventFn.SetData((char*) &in);
						InputDesktopSelected();
						counter=0;
						break;
					case WAIT_TIMEOUT:						
						counter++;
						break;
				}
		if (counter>3) break;
		}

error:
		return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{		
		// thread is needed, else setthreaddesktop fail
		HANDLE ThreadHandle2;
		DWORD dwTId;
		ThreadHandle2 = CreateThread(NULL, 0, Cadthread, NULL, 0, &dwTId);
		WaitForSingleObject(ThreadHandle2,INFINITE);
		CloseHandle(ThreadHandle2);		
}



