// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


//#include "stdhdrs.h"
#include "inifile.h"
#pragma warning( disable : 4100 )

IniFile::IniFile()
{
}

void IniFile::setIniFile(char* iniFile)
{
	strcpy_s(strIniFile, iniFile);
}

IniFile::~IniFile()
{
}

bool
IniFile::WriteString(const char*key1, const char*key2,char *value)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),strIniFile);
	return (FALSE != WritePrivateProfileString(key1,key2, value,strIniFile));
}

bool
IniFile::WriteInt(const char*key1, const char*key2,int value)
{
	char       buf[32];
	wsprintf(buf, "%d", value);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),strIniFile);
	int result=WritePrivateProfileString(key1,key2, buf,strIniFile);
	if (result==0) return false;
	return true;
}

int
IniFile::ReadInt(const char*key1, const char*key2,int Defaultvalue)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),strIniFile);
	return GetPrivateProfileInt(key1, key2, Defaultvalue, strIniFile);
}

void
IniFile::ReadString(const char*key1, const char*key2,char *value,int valuesize)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),strIniFile);
	GetPrivateProfileString(key1,key2, "",value,valuesize,strIniFile);
}

void
IniFile::ReadPassword(char *value,int valuesize)
{
	GetPrivateProfileStruct("UltraVNC","passwd",value,8,strIniFile);
}

void
IniFile::ReadPasswordViewOnly(char *value,int valuesize) //PGM
{ 
	GetPrivateProfileStruct("UltraVNC","passwd2",value,8,strIniFile); //PGM
}

bool
IniFile::WritePassword(char *value)
{
	if (strlen(value) == 0)
		return (FALSE != WritePrivateProfileStruct("UltraVNC", "passwd", NULL, 8, strIniFile));
	return (FALSE != WritePrivateProfileStruct("UltraVNC","passwd", value,8,strIniFile));
}

bool 
IniFile::WritePasswordViewOnly(char*value)
{ 
	if (strlen(value) == 0)
		return (FALSE != WritePrivateProfileStruct("UltraVNC", "passwd2", NULL, 8, strIniFile));
	return (FALSE != WritePrivateProfileStruct("UltraVNC","passwd2", value,8,strIniFile));
} 

bool IniFile::IsWritable()
{
    bool writable = WriteInt("Permissions", "isWritable",1);
    if (writable)
        WritePrivateProfileSection("Permissions", "", strIniFile);

    return writable;
}

void
IniFile::ReadHash(char* value, int valuesize)
{
	GetPrivateProfileStruct("UltraVNC", "hash", value, valuesize, strIniFile);
}

bool
IniFile::WriteHash(char* value, int valuesize)
{
	return (FALSE != WritePrivateProfileStruct("UltraVNC", "hash", value, valuesize, strIniFile));
}