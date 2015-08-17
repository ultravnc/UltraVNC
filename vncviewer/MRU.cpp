/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////

//
// MRU maintains a list of 'Most Recently Used' strings in the registry
// 

#include "MRU.h"
#include "VNCOptions.h"
#include <shlobj.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>

static const TCHAR * INDEX_VAL_NAME = _T("index");
static const int MRU_MAX_ITEM_LENGTH = 256;

MRU::MRU(LPTSTR keyname, unsigned int maxnum)
{
    m_index[0] = _T('\0');
    m_maxnum = maxnum;

    // Read the index entry
    ReadIndex();

    //Tidy();
}

// Add the item specified at the front of the list
// Move it there if not already.  If this makes the
// list longer than the maximum, older ones are deleted.

void ofnInit();
void MRU::AddItem(LPTSTR txt) 
{
    //if (m_hRegKey == NULL) return;

	// We don't add empty items.
	if (_tcslen(txt) == 0) return;

    // Read each value in index,
    // noting which is the first unused id
    TCHAR id = _T('\0');
    TCHAR firstUnusedId = _T('A');
    TCHAR itembuf[MRU_MAX_ITEM_LENGTH+1];

	// Find first unused letter.
	while (_tcschr(m_index, firstUnusedId) != NULL)
		firstUnusedId++;

    for (int i = 0; i < (int) _tcslen(m_index); i++) {
        
        // Does this entry already contain the item we're adding
        if (GetItem(i, itembuf, MRU_MAX_ITEM_LENGTH) != 0) {
            
            // If a value matches the txt specified, move it to the front.
            if (_tcscmp(itembuf, txt) == 0) {
                id = m_index[i];
                for (int j = i; j > 0; j--)
                    m_index[j] = m_index[j-1];
                m_index[0] = id;
                WriteIndex();
                // That's all we need to do.
                return;
            }
        }
    }

    // If we haven't found it, then we need to create a new entry and put it
    // at the front of the index.
    TCHAR valname[2];
    valname[0] = firstUnusedId;
    valname[1] = _T('\0');


	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	WritePrivateProfileString("connection", valname, txt, fname);


    /*RegSetValueEx(m_hRegKey, valname, NULL, REG_SZ, 
        (CONST BYTE *) txt, (_tcslen(txt) + 1) * sizeof(TCHAR));*/
    
    // move all the current ids up one
    for (int j = _tcslen(m_index); j >= 0; j--)
        m_index[j] = m_index[j-1];
    
    // and insert this one at the front
    m_index[0] = firstUnusedId;

    WriteIndex();

    // Tidy, to truncate index if too long.
    //Tidy();
}

// How many items are on the list?
int MRU::NumItems()
{
//    if (m_hRegKey == NULL) return 0;

    // return the length of index
    return _tcslen(m_index);
}

// Return them in order. 0 is the newest.
// NumItems()-1 is the oldest.
// Returns length, or 0 if unsuccessful.
int MRU::GetItem(int index, LPTSTR buf, int buflen)
{
//    if (m_hRegKey == NULL)      return 0;
    if (index > NumItems() - 1) return 0;

    TCHAR valname[2];
    valname[0] = m_index[index];
    valname[1] = _T('\0');

    DWORD dwbuflen = buflen;

	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	GetPrivateProfileString("connection", valname, "",buf, buflen, fname);


    /*if ( RegQueryValueEx( m_hRegKey,  valname, 
            NULL, &valtype, 
            (LPBYTE) buf, &dwbuflen) != ERROR_SUCCESS)
          return 0;

    if (valtype != REG_SZ)
        return 0;  // should really be an assert*/

    // May not be one byte per char, so we won't use dwbuflen
    return _tcslen(buf);
}

// Remove the specified item if it exists.
// Only one copy will be removed, but nothing should occur more than once.
void MRU::RemoveItem(LPTSTR txt)
{
    //if (m_hRegKey == NULL) return;

    TCHAR itembuf[MRU_MAX_ITEM_LENGTH+1];

    for (int i = 0; i < NumItems(); i++) {
        GetItem(i, itembuf, MRU_MAX_ITEM_LENGTH);
        if (_tcscmp(itembuf, txt) == 0) {
            RemoveItem(i);
            break;
        }
    }

}

// Remove the item with the given index.
// If this is greater than NumItems()-1 it will be ignored.
void MRU::RemoveItem(int index)
{
    //if (m_hRegKey == NULL) return;
    if (index > NumItems()-1) return;

    TCHAR valname[2];
    valname[0] = m_index[index];
    valname[1] = _T('\0');

	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	WritePrivateProfileString("connection", valname, NULL, fname);

    //RegDeleteValue(m_hRegKey, valname);

    for (unsigned int i = index; i <= _tcslen(m_index); i++)
        m_index[i] = m_index[i+1];
    
    WriteIndex();
}

// Load the index string from the registry
void MRU::ReadIndex()
{
    //if (m_hRegKey == NULL) return;

    // read the index
    DWORD dwindexlen = sizeof(m_index);

	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	if (GetPrivateProfileString("connection", INDEX_VAL_NAME, "", m_index, dwindexlen, fname) == 0) WriteIndex();


    /*if (RegQueryValueEx( m_hRegKey, INDEX_VAL_NAME,
        NULL, &valtype, (LPBYTE) m_index, &dwindexlen) == ERROR_SUCCESS) {
    } else {
        // If index entry doesn't exist, create it
        WriteIndex();
    }*/
}

// Save the index string to the registry
void MRU::WriteIndex()
{
   //if (m_hRegKey == NULL) return;

	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	WritePrivateProfileString("connection", INDEX_VAL_NAME, m_index, fname);

   /*RegSetValueEx(m_hRegKey, INDEX_VAL_NAME, NULL, REG_SZ, 
        (CONST BYTE *)m_index, (_tcslen(m_index) + 1) * sizeof(TCHAR));*/
}

// Tidy is called from time to time to preserve the integrity of the MRU
// list.  It does three things:
//  * Check the length and integrity of the index
//  * Check that all other values in the registry key have an 
//    entry in the index and delete them from registry if not.
//  * Check that all entries in the index have a corresponding
//    value in the registry key and delete them from index if not.

/*void MRU::Tidy()
{
   // if (m_hRegKey == NULL) return;
    int i;    

	// First some checks on the index itself.
    // Truncate the index.
    m_index[m_maxnum] = _T('\0');
    // Check that no entry occurs more than once.
	DWORD seenCharMask = 0;
	for (i = 0; m_index[i] != _T('\0'); i++) {
		DWORD mask = 1 << (m_index[i]-_T('A'));
		if (seenCharMask & mask) {
			// The current character has been used before.
			// Delete it and move everything up.
			for (int j = i; m_index[j] != _T('\0'); j++)
				m_index[j] = m_index[j+1];
			// Start next check at new current character
			i--;
		}
		seenCharMask |= mask;
	}


	// Then check that all entries in registry have entry in index.
    TCHAR valname[256];
    DWORD valtype=0;
    DWORD valnamelen=0;
    DWORD numValues=0;

    RegQueryInfoKey ( m_hRegKey,  
        NULL, NULL, 
        NULL, NULL, 
        NULL, NULL,  
        &numValues,  
        NULL, NULL,
        NULL, NULL);

    // We're being good here.  The documentation says we shouldn't
    // modify a key while enumerating its values. So this array will
    // hold the names of keys which should be deleted
    TCHAR **dudValues = new TCHAR* [numValues];
    for (i = 0; i < (int) numValues; i++)
        dudValues[i] = NULL;
    unsigned int numDudValues = 0;

    i = 0;
    while (1) {
        valnamelen = 255;
        
        if (RegEnumValue(m_hRegKey, i, 
                        valname, &valnamelen, NULL,
                        &valtype, NULL, NULL) == ERROR_NO_MORE_ITEMS)
            break;

        i++;
        
        // ignore the index
        if (_tcscmp(valname, INDEX_VAL_NAME) == 0)
            continue;
        // Record as invalid other non-single-char entries
        if (_tcslen(valname) > 1) {
            dudValues[numDudValues++] = _tcsdup(valname);
            continue;
        }
        // Record as invalid if not in the index
        if (_tcschr(m_index, valname[0]) == 0) {
            dudValues[numDudValues++] = _tcsdup(valname);
            continue;
        }   
    }

    // Now delete the invalid ones
    for (i = 0; i < (int) numDudValues; i++) {
        RegDeleteValue(m_hRegKey, dudValues[i]);
        free(dudValues[i]);
    }
    delete[] dudValues;

    // Lastly, check that all entries in the index have a corresponding 
    // entry in registry and delete them if not.
    for (i = _tcslen(m_index)-1; i >= 0;  i--) {
        TCHAR itembuf[MRU_MAX_ITEM_LENGTH+1];
        if (GetItem(i, itembuf, MRU_MAX_ITEM_LENGTH) == 0) {
            for (unsigned int j = i; j <= _tcslen(m_index)-1; j++)
                m_index[j] = m_index[j+1];
        }
    }

	// Save any changes to the index.
    WriteIndex();
}*/


MRU::~MRU()
{
    /*if (m_hRegKey != NULL) {
        Tidy();
        RegCloseKey(m_hRegKey);
        m_hRegKey = NULL;
    }*/
}
