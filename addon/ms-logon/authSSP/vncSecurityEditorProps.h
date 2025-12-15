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


// This table maps permissions onto strings.
// Notice the first entry, which includes all permissions; it maps to a string "Full Control".
// I love this feature of being able to provide multiple permissions in one entry,
// if you use it carefully, it's pretty powerful - the access control editor will
// automatically keep the permission checkboxes synchronized, which makes it clear
// to the user what is going on (if they pay close enough attention).
//
// Notice that I included standard permissions - don't forget them...
// Notice that I left some of the more esoteric permissions for the Advanced dialog,
// and that I only show "Full Control" in the basic permissions editor.
// This is consistent with the way the file acl editor works.
SI_ACCESS g_vncAccess[] = {	
	// these are a easy-to-swallow listing of basic rights for VNC
//	{ &GUID_NULL, 0x00000007, L"Full control", SI_ACCESS_GENERAL  },	// Full Control
	{ &GUID_NULL, 0x00000003, L"Full control", SI_ACCESS_GENERAL  },	// Full Control
	{ &GUID_NULL, 0x00000003, L"Interact",     SI_ACCESS_GENERAL  },	// Write
	{ &GUID_NULL, 0x00000001, L"View",     SI_ACCESS_GENERAL  },	// Read
//	{ &GUID_NULL, 0x00000004, L"File Transfer",     SI_ACCESS_GENERAL  },	// ??
};

// Here's my crufted-up mapping for VNC generic rights
GENERIC_MAPPING g_vncGenericMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	STANDARD_RIGHTS_REQUIRED
};

const DWORD ViewOnly	   = 0x0001;
const DWORD Interact	   = 0x0002;

const DWORD GenericRead    = STANDARD_RIGHTS_READ |
							 ViewOnly;

const DWORD GenericWrite   = STANDARD_RIGHTS_WRITE |
							 Interact;

const DWORD GenericExecute = STANDARD_RIGHTS_EXECUTE;

const DWORD GenericAll     = STANDARD_RIGHTS_REQUIRED |
							  GenericRead |
							  GenericWrite |
							  GenericExecute;

