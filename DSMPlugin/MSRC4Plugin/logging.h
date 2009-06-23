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

#ifndef _LOGGING_H
#define _LOGGING_H

#pragma once
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include "utils.h"

#define BUFFER_SIZE 256
#define APPEND 1
#define LOGNAME_SIZE 32


extern HANDLE hLogFile;
extern long LOGIT;	//flag to turn on/off logging
extern long DEBUGIT;	//flag to turn on/off additional logging for crypto.exe

void PrintIt(const char *);
void DebugIt(const char *);
int SetLogging(const char * logname);
int SetLogging(const char * logname, char * sLogit);
int SetLogStatus(const char * sLogit);
void SetLogFile(const char * logname);

#define DEST buff

//Macros for logging

#define PrintLog(x)						\
	{									\
		char buff[4096];				\
		sprintf x;						\
		PrintIt(buff);					\
	}

#define DebugLog(x)						\
	{									\
		char buff[4096];				\
		sprintf x;						\
		DebugIt(buff);					\
	}


#endif