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
/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file is part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
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
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.

//
// REGISTRY class allows editing of strings in the registry
// 
#include "registry.h"



static const int REGISTRY_MAX_ITEM_LENGTH = 256;

REGISTRY::REGISTRY(HKEY hivename, LPTSTR keyname, bool blnWritable)
{
int rc = 0;
DWORD dispos;

	//PrintLog((DEST,"REGISTRY Opening hive"));

	if (blnWritable == true)
	{
		// Create the registry key.  If unsuccessful all other methods will be no-ops.
		rc = RegCreateKeyEx(hivename, keyname, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &m_hRegKey, &dispos);
		if (rc != ERROR_SUCCESS ) {
#ifdef _WITH_LOG  
			DebugLog((DEST,"ReadItem failed"));
#endif  
			m_hRegKey = NULL;
		}
    }
	else
	{
		// Create the registry key.  If unsuccessful all other methods will be no-ops.
		rc = RegCreateKeyEx(hivename, keyname, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &m_hRegKey, &dispos);
		if (rc != ERROR_SUCCESS ) {
#ifdef _WITH_LOG  
			DebugLog((DEST,"ReadItem failed"));
#endif  
			m_hRegKey = NULL;
			blnOpen = false;
		}
		blnOpen = true;
	}
	writable = blnWritable;
}


// Load the index string from the registry
BOOL REGISTRY::ReadItem(LPSTR keyValue, DWORD dwindexlen, const TCHAR * indexVal, const TCHAR * defaultVal)
{
int rc = 0;
BOOL bRC = false;

    if (m_hRegKey == NULL) return false;

    // read the index
    DWORD valtype;
	valtype = REG_SZ;
    //DWORD dwindexlen = sizeof(keyValue);
	rc = RegQueryValueEx( m_hRegKey, indexVal, NULL, &valtype, (LPBYTE) keyValue, &dwindexlen); 
    if (rc == ERROR_SUCCESS) 
	{
		return true;
    } 
	else 
	{
		if (rc ==  ERROR_MORE_DATA)
		{
#ifdef _WITH_LOG  
			DebugLog((DEST,"ReadItem failed - indexVal too small"));
#endif  
		}
		else
		{
#ifdef _WITH_LOG  
			DebugLog((DEST,"ReadItem - missing key %s", indexVal));
#endif  

			// If index entry doesn't exist, create it
			if (writable)
			{
			   bRC = WriteItem((char *)defaultVal, indexVal);
				strcpy(keyValue, defaultVal);
			}
		}
    }

	return bRC;
}

// Save the index string to the registry
BOOL REGISTRY::WriteItem(LPSTR keyValue, const TCHAR* indexVal)
{
int rc = 0;

   if (m_hRegKey == NULL) return false;

   rc = RegSetValueEx(m_hRegKey, indexVal, NULL, REG_SZ, (CONST BYTE *)keyValue, strlen(keyValue));
  
    if (rc == ERROR_SUCCESS) 
		return true;
	else
	{
#ifdef _WITH_LOG  
		DebugLog((DEST,"WriteItem %s failed", indexVal));
#endif  
		return false;
	}
}


REGISTRY::~REGISTRY()
{
    if (m_hRegKey != NULL) {
		
		//PrintLog((DEST,"~REGISTRY Closing hive"));
        RegCloseKey(m_hRegKey);
        m_hRegKey = NULL;
    }
}

