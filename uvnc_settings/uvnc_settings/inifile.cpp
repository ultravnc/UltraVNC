#include "stdafx.h"
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
	strcat(myInifile,INIFILE_NAME);
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
	strcat(myInifile,INIFILE_NAME);
}

void
IniFile::IniFileSetTemp(char *lpCmdLine)
{	
	strcpy(myInifile,lpCmdLine);
}

IniFile::~IniFile()
{
}

bool
IniFile::WriteString(char *key1, char *key2,char *value)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile); 
	return (FALSE != WritePrivateProfileString(key1,key2, value,myInifile));
}

bool 
IniFile::WriteInt(char *key1, char *key2,int value)
{
	char       buf[32];
	wsprintf(buf, "%d", value);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	int result=WritePrivateProfileString(key1,key2, buf,myInifile);
	if (result==0) return false;
	return true;
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
	//int size=ReadInt("UltraVNC", "passwdsize",0);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifilePasswd);
	GetPrivateProfileStruct("UltraVNC","passwd",value,8,myInifile);
}

bool
IniFile::WritePassword(char *value)
{
		//WriteInt("UltraVNC", "passwdsize",sizeof(value));
		//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
		return (FALSE != WritePrivateProfileStruct("UltraVNC","passwd", value,8,myInifile));
}

void //PGM
IniFile::ReadPassword2(char *value,int valuesize) //PGM
{ //PGM
	GetPrivateProfileStruct("UltraVNC","passwd2",value,8,myInifile); //PGM
} //PGM

bool //PGM
IniFile::WritePassword2(char *value) //PGM
{ //PGM
		return (FALSE != WritePrivateProfileStruct("UltraVNC","passwd2", value,8,myInifile)); //PGM
} //PGM

bool IniFile::IsWritable()
{
    bool writable = WriteInt("Permissions", "isWritable",1);
    if (writable)
        WritePrivateProfileSection("Permissions", "", myInifile);

    return writable;
}

