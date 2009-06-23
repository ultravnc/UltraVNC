#ifndef _REGISTRY_H
#define _REGISTRY_H

//  Copyright (C) 2005 Sean E. Covel All Rights Reserved.
//
//  Created by Sean E. Covel based on UltraVNC's excellent TestPlugin project.
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
// REGISTRY class is to allow read/write of strings in the registry
// 

#ifdef _WITH_LOG  
	#pragma once
	#include "logging.h"
#else 
	#include <windows.h>  
#endif  


class REGISTRY  
{
public:

    // Create an REGISTRY object and initialise it from the key.
    // Key created if not existant.

	bool blnOpen;

    REGISTRY(HKEY hivename, LPTSTR keyname, bool blnWriteble);

	BOOL ReadItem(LPSTR keyValue, DWORD keyLength, const TCHAR * indexVal, const TCHAR * defaultVal);
	BOOL WriteItem(LPSTR keyValue, const TCHAR * indexVal);
	
	virtual ~REGISTRY();

private:

    HKEY m_hRegKey;
	bool writable;

protected:
	
};



#endif