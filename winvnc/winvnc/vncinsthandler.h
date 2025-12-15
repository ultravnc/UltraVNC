// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncInstHandler

// The WinMain procedure for UltraVNC Server produces one of these objects.
// It creates a named mutex and checks to see whether that mutex
// already existed in the system. If it did, then there is a previous
// instance of UltraVNC Server running, which must be requested to quit cleanly.

class vncInstHandler;

#if (!defined(_WINVNC_VNCINSTHANDLER))
#define _WINVNC_VNCINSTHANDLER

// Includes
#include "stdhdrs.h"

// The vncInstHandler class itself
class vncInstHandler
{
	
public:
	vncInstHandler();
	~vncInstHandler();
	// Constructor/destructor
	BOOL Init();
	HANDLE mutex;
};

#endif // _WINVNC_VNCINSTHANDLER
