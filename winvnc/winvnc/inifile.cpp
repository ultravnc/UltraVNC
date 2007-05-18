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

IniFile::~IniFile()
{
}

bool
IniFile::WriteString(char *key1, char *key2,char *value)
{
	return WritePrivateProfileString(key1,key2, value,myInifile);
}

bool 
IniFile::WriteInt(char *key1, char *key2,int value)
{
	char       buf[32];
	wsprintf(buf, "%d", value);
	return WritePrivateProfileString(key1,key2, buf,myInifile);
}

int
IniFile::ReadInt(char *key1, char *key2,int Defaultvalue)
{
	return GetPrivateProfileInt(key1, key2, Defaultvalue, myInifile);
}

void 
IniFile::ReadString(char *key1, char *key2,char *value,int valuesize)
{
GetPrivateProfileString(key1,key2, "",value,valuesize,myInifile);
}

void 
IniFile::ReadPassword(char *value,int valuesize)
{
	//int size=ReadInt("ultravnc", "passwdsize",0);
	GetPrivateProfileStruct("ultravnc","passwd",value,8,myInifile);
}

bool
IniFile::WritePassword(char *value)
{
		//WriteInt("ultravnc", "passwdsize",sizeof(value));
		return WritePrivateProfileStruct("ultravnc","passwd", value,8,myInifile);
}