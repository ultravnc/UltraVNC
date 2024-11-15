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
#include <winsock2.h>
#include <windows.h>
#if (!defined(_WINVNC_INIFILE))
#define _WINVNC_INIFILE

#ifndef SC_20
	#define INIFILE_NAME "ultravnc.ini"
#else
	#define INIFILE_NAME "SC_20.ini"
#endif // SC_20

class IniFile
{

// Fields
public:
	char myInifile[MAX_PATH];

// Methods
public:
	// Make the desktop thread & window proc friends

	IniFile();
	~IniFile();
	bool WriteString(const char*key1, const char*key2,char *value);
	bool WritePassword(char*value);
	bool WritePasswordViewOnly(char*value); //PGM
	bool WriteInt(const char*key1, const char*key2,int value);
	int ReadInt(const char*key1, const char*key2,int Defaultvalue);
	void ReadString(const char*key1, const char*key2,char *value,int valuesize);
	void ReadPassword(char *value,int valuesize);
	void ReadPasswordViewOnly(char *value,int valuesize); //PGM

	void setIniFile(char* iniFile);

    bool IsWritable();
	void ReadHash(char* value, int valuesize);
	bool WriteHash(char* value, int valuesize);

protected:
		
};
#endif
