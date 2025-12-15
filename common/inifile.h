// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


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
	char strIniFile[MAX_PATH];

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
