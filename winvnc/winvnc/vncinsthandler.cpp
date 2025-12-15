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


// vncInstHandler.cpp

// Implementation of the class used to ensure that only
// one instance is running

#include "stdhdrs.h"
#include "vncinsthandler.h"

// Name of the mutex

const char mutexname [] = "WinVNC_Win32_Instance_Mutex";

// The class methods
vncInstHandler::vncInstHandler()
{
	mutex=NULL;
}

vncInstHandler::~vncInstHandler()
{
	if (mutex)
	{
		ReleaseMutex(mutex);
		CloseHandle (mutex);
	}
}

BOOL
vncInstHandler::Init()
{
	// Create the named mutex
	mutex = CreateMutex(NULL, FALSE, mutexname);
	if (mutex == NULL)
		return FALSE;

	// Check that the mutex didn't already exist
	if (GetLastError() == ERROR_ALREADY_EXISTS)
		return FALSE;

	return TRUE;
}
