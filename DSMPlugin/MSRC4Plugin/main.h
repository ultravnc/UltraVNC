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

#ifndef _MAIN_H
#define _MAIN_H


#pragma once
#include <windows.h>
#include "EnvReg.h"

int Encrypt(char * inFile, char * outFile, char * keyFile);
int Decrypt(char * inFile, char * outFile, char * keyFile);
int CreateKey(char * keyFile, DWORD keyLen);
int ListProviders();
int WriteDebugInfo();
void Usage();
void Version();
int WinVer();


extern char CSP_NAME[70];
extern DWORD VERIFY_CONTEXT_FLAG;
extern DWORD NULL_CONTEXT_FLAG;
extern DWORD CONTEXT_FLAG;
extern DWORD KEYLEN;
extern DWORD MAXKEYLEN;
extern CHAR szUserName[100];         // Buffer to hold the name of the key container.


#endif
