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
}

// Add the item specified at the front of the list
// Move it there if not already.  If this makes the
// list longer than the maximum, older ones are deleted.

void ofnInit();
void MRU::AddItem(LPTSTR txt) 
{
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
    
    // move all the current ids up one
    for (int j = _tcslen(m_index); j >= 0; j--)
        m_index[j] = m_index[j-1];
    
    // and insert this one at the front
    m_index[0] = firstUnusedId;

    WriteIndex();
}

// How many items are on the list?
int MRU::NumItems()
{
    // return the length of index
    return _tcslen(m_index);
}

// Return them in order. 0 is the newest.
// NumItems()-1 is the oldest.
// Returns length, or 0 if unsuccessful.
int MRU::GetItem(int index, LPTSTR buf, int buflen)
{
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

    // May not be one byte per char, so we won't use dwbuflen
    return _tcslen(buf);
}

// Remove the specified item if it exists.
// Only one copy will be removed, but nothing should occur more than once.
void MRU::RemoveItem(LPTSTR txt)
{
    TCHAR itembuf[MRU_MAX_ITEM_LENGTH+1];

    for (int i = 0; i < NumItems(); i++) {
        GetItem(i, itembuf, MRU_MAX_ITEM_LENGTH);
        if (_tcscmp(itembuf, txt) == 0) {
            RemoveItem(i);
            break;
        }
    }

}

void MRU::SetPos(LPTSTR txt, int x, int y, int w, int h)
{
	char buf[32];
	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	sprintf(buf, "%d", x);
	WritePrivateProfileString(txt, "x", buf, fname);
	sprintf(buf, "%d", y);
	WritePrivateProfileString(txt, "y", buf, fname);
	sprintf(buf, "%d", w);
	WritePrivateProfileString(txt, "w", buf, fname);
	sprintf(buf, "%d", h);
	WritePrivateProfileString(txt, "h", buf, fname);
}

int MRU::Get_x(LPTSTR txt)
{
	char buf[32];
	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	GetPrivateProfileString(txt, "x", "", buf, 32, fname);
	return atoi(buf);
}
int MRU::Get_y(LPTSTR txt)
{
	char buf[32];
	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	GetPrivateProfileString(txt, "y", "", buf, 32, fname);
	return atoi(buf);
}
int MRU::Get_w(LPTSTR txt)
{
	char buf[32];
	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	GetPrivateProfileString(txt, "w", "", buf, 32, fname);
	return atoi(buf);
}
int MRU::Get_h(LPTSTR txt)
{
	char buf[32];
	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	GetPrivateProfileString(txt, "h", "", buf, 32, fname);
	return atoi(buf);
}
// Remove the item with the given index.
// If this is greater than NumItems()-1 it will be ignored.
void MRU::RemoveItem(int index)
{
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

    for (unsigned int i = index; i <= _tcslen(m_index); i++)
        m_index[i] = m_index[i+1];
    
    WriteIndex();
}

// Load the index string from the registry
void MRU::ReadIndex()
{
    // read the index
    DWORD dwindexlen = sizeof(m_index);

	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	if (GetPrivateProfileString("connection", INDEX_VAL_NAME, "", m_index, dwindexlen, fname) == 0) WriteIndex();
}

// Save the index string to the registry
void MRU::WriteIndex()
{
	char fname[_MAX_PATH];
	ofnInit();
	char optionfile[MAX_PATH];
	VNCOptions::GetDefaultOptionsFileName(optionfile);
	sprintf(fname, optionfile);
	WritePrivateProfileString("connection", INDEX_VAL_NAME, m_index, fname);
}


MRU::~MRU()
{
    
}
