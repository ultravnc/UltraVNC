// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include <windows.h>
#if (!defined(_WINVNC_INIFILE))
#define _WINVNC_INIFILE

#define INIFILE_NAME "ultravnc.ini"

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
	bool WriteString(char *key1, char *key2,char *value);
	bool WritePassword(char *value);
	bool WritePassword2(char *value); //PGM
	bool WriteInt(char *key1, char *key2,int value);
	int ReadInt(char *key1, char *key2,int Defaultvalue);
	void ReadString(char *key1, char *key2,char *value,int valuesize);
	bool ReadPassword(char *value,int valuesize);
	bool ReadPassword2(char *value,int valuesize); //PGM
	void IniFileSetSecure();
	//void IniFileSetTemp();
	void IniFileSetTemp(char *lpCmdLine);
	void copy_to_secure();

    bool IsWritable();

protected:
};
#endif