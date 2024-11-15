/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
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
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


//#include "stdhdrs.h"
#include "inifile.h"
#pragma warning( disable : 4100 )

IniFile::IniFile()
{
}

void IniFile::setIniFile(char* iniFile)
{
	strcpy_s(myInifile, iniFile);
}

IniFile::~IniFile()
{
}

bool
IniFile::WriteString(const char*key1, const char*key2,char *value)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	return (FALSE != WritePrivateProfileString(key1,key2, value,myInifile));
}

bool
IniFile::WriteInt(const char*key1, const char*key2,int value)
{
	char       buf[32];
	wsprintf(buf, "%d", value);
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	int result=WritePrivateProfileString(key1,key2, buf,myInifile);
	if (result==0) return false;
	return true;
}

int
IniFile::ReadInt(const char*key1, const char*key2,int Defaultvalue)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	return GetPrivateProfileInt(key1, key2, Defaultvalue, myInifile);
}

void
IniFile::ReadString(const char*key1, const char*key2,char *value,int valuesize)
{
	//vnclog.Print(LL_INTERR, VNCLOG("%s \n"),myInifile);
	GetPrivateProfileString(key1,key2, "",value,valuesize,myInifile);
}

void
IniFile::ReadPassword(char *value,int valuesize)
{
	GetPrivateProfileStruct("UltraVNC","passwd",value,8,myInifile);
}

void
IniFile::ReadPasswordViewOnly(char *value,int valuesize) //PGM
{ 
	GetPrivateProfileStruct("UltraVNC","passwd2",value,8,myInifile); //PGM
}

bool
IniFile::WritePassword(char *value)
{
	if (strlen(value) == 0)
		return (FALSE != WritePrivateProfileStruct("UltraVNC", "passwd", NULL, 8, myInifile));
	return (FALSE != WritePrivateProfileStruct("UltraVNC","passwd", value,8,myInifile));
}

bool 
IniFile::WritePasswordViewOnly(char*value)
{ 
	if (strlen(value) == 0)
		return (FALSE != WritePrivateProfileStruct("UltraVNC", "passwd2", NULL, 8, myInifile));
	return (FALSE != WritePrivateProfileStruct("UltraVNC","passwd2", value,8,myInifile));
} 

bool IniFile::IsWritable()
{
    bool writable = WriteInt("Permissions", "isWritable",1);
    if (writable)
        WritePrivateProfileSection("Permissions", "", myInifile);

    return writable;
}

void
IniFile::ReadHash(char* value, int valuesize)
{
	GetPrivateProfileStruct("UltraVNC", "hash", value, valuesize, myInifile);
}

bool
IniFile::WriteHash(char* value, int valuesize)
{
	return (FALSE != WritePrivateProfileStruct("UltraVNC", "hash", value, valuesize, myInifile));
}