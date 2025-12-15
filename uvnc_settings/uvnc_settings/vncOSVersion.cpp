// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#include "stdafx.h"

#include <windows.h>
int OSTYPE=4;
void
SetOSVersion()
{
	OSVERSIONINFO OSversion;
	
	OSversion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);

	GetVersionEx(&OSversion);

	switch(OSversion.dwPlatformId)
	{
		case VER_PLATFORM_WIN32_NT:
								  if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==0)
									 OSTYPE=1;							    
								  if(OSversion.dwMajorVersion==5 && OSversion.dwMinorVersion==1)
									 OSTYPE=1;
								  if(OSversion.dwMajorVersion==5)
									 OSTYPE=1;
								  if(OSversion.dwMajorVersion==6)
									 OSTYPE=2;
								  if(OSversion.dwMajorVersion<=4) 	  
								     OSTYPE=3;
								  break;
		case VER_PLATFORM_WIN32_WINDOWS:
								if(OSversion.dwMinorVersion==0) 
								{
									OSTYPE=5; //95
									break;
								}
								OSTYPE=4;
	}
}

int OSversion()
{
	return OSTYPE;
}