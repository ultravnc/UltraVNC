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


// REGISTRY class is to allow read/write of strings in the registry
// 

#ifndef _REGISTRY_H
#define _REGISTRY_H

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