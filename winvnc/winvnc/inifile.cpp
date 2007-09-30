#include "stdhdrs.h"
#include "inifile.h"

IniFile::IniFile()
{
char WORKDIR[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return;
		*p = '\0';
		}
	strcpy(myInifile,"");
	strcat(myInifile,WORKDIR);//set the directory
	strcat(myInifile,"\\");
	strcat(myInifile,"ultravnc.ini");
}

void
IniFile::IniFileSetSecure()
{
char WORKDIR[MAX_PATH];
	if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return;
		*p = '\0';
		}
	strcpy(myInifile,"");
	strcat(myInifile,WORKDIR);//set the directory
	strcat(myInifile,"\\");
	strcat(myInifile,"ultravnc.ini");
}

void
IniFile::IniFileSetTemp()
{
char WORKDIR[MAX_PATH];

	if (!GetTempPath(MAX_PATH,WORKDIR))
	{
		//Function failed, just set something
		if (GetModuleFileName(NULL, WORKDIR, MAX_PATH))
		{
		char* p = strrchr(WORKDIR, '\\');
		if (p == NULL) return;
		*p = '\0';
		}
		strcpy(myInifile,"");
		strcat(myInifile,WORKDIR);//set the directory
		strcat(myInifile,"\\");
		strcat(myInifile,"ultravnc.ini");
		return;
	}

	strcpy(myInifile,"");
	strcat(myInifile,WORKDIR);//set the directory
	strcat(myInifile,"ultravnc.ini");
}

void
IniFile::copy_to_secure()
{
/*SHELLEXECUTEINFO shExecInfo;

      shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

      shExecInfo.fMask = NULL;
      shExecInfo.hwnd = GetForegroundWindow();
      shExecInfo.lpVerb = "runas";
      shExecInfo.lpFile = "UacVista.exe";
      shExecInfo.lpParameters = myInifile;
      shExecInfo.lpDirectory = NULL;
      shExecInfo.nShow = SW_SHOWNORMAL;
      shExecInfo.hInstApp = NULL;
	  ShellExecuteEx(&shExecInfo);*/
	  //ShellExecute(GetForegroundWindow(), "open", "UacVista.exe", myInifile , 0, SW_SHOWNORMAL);


        char dir[MAX_PATH], *ptr;
		GetModuleFileName(0, dir, MAX_PATH);

		ptr=strrchr(dir, '\\'); 
		if(ptr)
			ptr[1]='\0'; 
		if(!SetCurrentDirectory(dir)) {
        return ;
		}
		strcat(dir, "\\uacvistahelper.exe");
		strcat(dir, " ");
		strcat(dir, myInifile);

		STARTUPINFO          StartUPInfo;
		PROCESS_INFORMATION  ProcessInfo;
		HANDLE Token=NULL;
		HANDLE process=NULL;
		ZeroMemory(&StartUPInfo,sizeof(STARTUPINFO));
		ZeroMemory(&ProcessInfo,sizeof(PROCESS_INFORMATION));
		StartUPInfo.wShowWindow = SW_SHOW;
		StartUPInfo.lpDesktop = "Winsta0\\Default";
		StartUPInfo.cb = sizeof(STARTUPINFO);
		HWND tray = FindWindow(("Shell_TrayWnd"), 0);
		if (!tray)
			return;
	
		DWORD processId = 0;
			GetWindowThreadProcessId(tray, &processId);
		if (!processId)
			return;	
		process = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
		if (!process)
			return;	
		OpenProcessToken(process, MAXIMUM_ALLOWED, &Token);
		CreateProcessAsUser(Token,NULL,dir,NULL,NULL,FALSE,DETACHED_PROCESS,NULL,NULL,&StartUPInfo,&ProcessInfo);
		if (process) CloseHandle(process);
		if (Token) CloseHandle(Token);

}

IniFile::~IniFile()
{
}

bool
IniFile::WriteString(char *key1, char *key2,char *value)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile); 
	return WritePrivateProfileString(key1,key2, value,myInifile);
}

bool 
IniFile::WriteInt(char *key1, char *key2,int value)
{
	char       buf[32];
	wsprintf(buf, "%d", value);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	return WritePrivateProfileString(key1,key2, buf,myInifile);
}

int
IniFile::ReadInt(char *key1, char *key2,int Defaultvalue)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	return GetPrivateProfileInt(key1, key2, Defaultvalue, myInifile);
}

void 
IniFile::ReadString(char *key1, char *key2,char *value,int valuesize)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	GetPrivateProfileString(key1,key2, "",value,valuesize,myInifile);
}

void 
IniFile::ReadPassword(char *value,int valuesize)
{
	//int size=ReadInt("ultravnc", "passwdsize",0);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifilePasswd);
	GetPrivateProfileStruct("ultravnc","passwd",value,8,myInifile);
}

bool
IniFile::WritePassword(char *value)
{
		//WriteInt("ultravnc", "passwdsize",sizeof(value));
		//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
		return WritePrivateProfileStruct("ultravnc","passwd", value,8,myInifile);
}


