//  Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//
//  Created by Sean E. Covel
//
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
// http://home.comcast.net/~msrc4plugin
// or
// mail: msrc4plugin@comcast.net
//
//
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _UTILS_H
#define _UTILS_H

#pragma once
#include <stdlib.h>
#include "registry.h"
#include "EnvReg.h"

#ifndef _WITH_REGISTRY
	#pragma once
	#include <stdio.h>
#endif  

	//Defines for the OS found in WinVer
#define WIN3_1	0
#define WIN95	1
#define WIN95B	2
#define WIN98	3
#define WIN98SE	4
#define WINME	5
#define WINNT	6
#define WIN2000	7
#define WINXP	8
#define WIN2003	9
#define WINVISTA	10 //pgm added

//Array of OS names in order of the defines above
static char WindowsName[][128] = {"Windows 3.1","Windows 95","Windows 95 osr2","Windows 98","Windows 98SE","Windows ME","Windows NT","Windows 2000","Windows XP","Windows 2003","Windows Vista"}; //pgm was 100, changed to 128 and added Vista

#define FILENAME_SIZE 256
#define OUTPUT_BUFFER_SIZE 2000

#define BufSize 256

int WhatWindowsVer(void);	//Get windows OS version

BOOL GetEnvVar(LPTSTR lpName, LPTSTR buffer, DWORD nSize);
int FindKey(const char* sPluginName, const char* sDefaultKeyName, const char* sVariable, HANDLE *hKeyFile, char* output);


#endif

