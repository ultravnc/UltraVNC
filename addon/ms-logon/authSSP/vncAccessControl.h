// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2004 Martin Scharpf. All Rights Reserved.
//


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#define ViewOnly 0x0001
#define Interact 0x0002

// 
class vncAccessControl
{
public:
	PSECURITY_DESCRIPTOR GetSD();
	BOOL SetSD(PSECURITY_DESCRIPTOR pSD);

protected:
	void FreeSD(PSECURITY_DESCRIPTOR pSD);
	PACL GetACL(void);
	BOOL StoreACL(PACL pACL);
	PSID GetOwnerSID(void);
};

