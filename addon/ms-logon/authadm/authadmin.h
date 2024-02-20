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


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the AUTHLOGONUSER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// AUTHLOGONUSER_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef AUTHADMIN_EXPORTS
#define AUTHADMIN_API __declspec(dllexport)
#else
#define AUTHADMIN_API __declspec(dllimport)
#endif
#pragma comment( lib, "netapi32.lib" )
#if defined( UNICODE ) || defined( _UNICODE )
#error Sorry -- please compile as an ANSI program.
#endif
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define MAXLEN 256

AUTHADMIN_API
BOOL CUGP(char * userin,char *password,char *machine,char * group,int locdom);
