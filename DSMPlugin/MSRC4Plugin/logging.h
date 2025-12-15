// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//


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