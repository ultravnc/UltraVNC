// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


//
// MRU maintains a list of 'Most Recently Used' strings in the registry
// 

#ifndef MRU_H__
#define MRU_H__

#pragma once
#include "stdhdrs.h"
#include <winsock2.h>
#include <windows.h>
#include <tchar.h>
#include "VNCOptions.h"

class MRU  
{
public:

    // Create an MRU and initialise it from the key.
    // Key created if not existant.

    MRU(LPTSTR keyname, unsigned int maxnum = 8);

    // Add the item specified at the front of the list
    // Move it there if not already. If this makes the
    // list longer than the maximum, older ones are deleted.
    void AddItem(LPTSTR txt);

    // How many items are on the list?
    int NumItems();

    // Return them in order. 0 is the newest.
    // NumItems()-1 is the oldest.
    int GetItem(int index, LPTSTR buf, int buflen);

    // Remove the specified item if it exists.
    void RemoveItem(LPTSTR txt);

    // Remove the item with the given index.
    // If this is greater than NumItems()-1 it will be ignored.
    void RemoveItem(int index);
	void SetPos(LPTSTR txt, int x, int y, int w, int h);
	int Get_x(LPTSTR txt);
	int Get_y(LPTSTR txt);
	int Get_w(LPTSTR txt);
	int Get_h(LPTSTR txt);

	virtual ~MRU();

private:
	void ReadIndex();

    TCHAR m_index[257] = {};  // allow a bit of workspace beyond maximum of 26
    unsigned int m_maxnum;
    TCHAR m_optionfile[MAX_PATH];
protected:
	void WriteIndex();
};

#endif
