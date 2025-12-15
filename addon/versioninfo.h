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


/* versioninfo.h: Defines 
	STR_COMPANYNAME, 
    STR_FILEVERSION,
	STR_PRODUCTVERSION,
	STR_COPYRIGHT,
	STR_SPECIALBUILD,
	INT_FILEVERSION,
	INT_PRODUCTVERSION */

#ifndef ULTRAVNC_VERSIONINFO_H
#define ULTRAVNC_VERSIONINFO_H

// ======================== Define Version here =======================
#define MAJ 1
#define MIN 0
#define V3 90
#define V4 0 // Used as ReleaseCandidate minor version in SpecialBuildDescription

// Set to 1 if ReleaseCandidate (toggles visibility of SpecialBuildDescription)
#define isRC 0

// =========== No further configuration needed below this line =================

// some helper macros (see http://http://gcc.gnu.org/onlinedocs/cpp/Stringification.html)
#define XSTR(x) STR(x)
#define STR(x) #x

// Concatenate tokens
#define CAT2(A,B) A.B
#define CAT4(A,B,C,D) A.B.C.D

#define INT_VERSION MAJ,MIN,V3,V4
#define STR_VERSION CAT4(MAJ,MIN,V3,V4)
#define STR_MAINVER CAT2(MAJ,MIN)
#define STR_RC_VERSION CAT2(V3,V4)

#define INT_FILEVERSION INT_VERSION
#define INT_PRODUCTVERSION INT_VERSION

#define STR_FILEVERSION XSTR(STR_VERSION) "\0"
#define STR_PRODUCTVERSION XSTR(STR_VERSION) "\0"
#define STR_COPYRIGHT "Copyright © 2002-2025 UltraVNC Team Members. All Rights Reserved.\0"
#define STR_SPECIALBUILD "v" XSTR(STR_MAINVER) " ReleaseCandidate " XSTR(STR_RC_VERSION) "\0"
#if isRC == 1
#define STR_ABOUT_SERVER_VERSION XSTR(UltraVNC Win32 Server v STR_MAINVER RC STR_RC_VERSION)
#define STR_ABOUT_VIEWER_VERSION XSTR(UltraVNC Win32 Viewer v STR_MAINVER RC STR_RC_VERSION)
#else
#define STR_ABOUT_SERVER_VERSION XSTR(UltraVNC Win32 Server v STR_VERSION)
#define STR_ABOUT_VIEWER_VERSION XSTR(UltraVNC Win32 Viewer v STR_VERSION)
#endif
#define STR_COMPANYNAME "UltraVNC\0"

#endif // ULTRAVNC_VERSIONINFO_H
