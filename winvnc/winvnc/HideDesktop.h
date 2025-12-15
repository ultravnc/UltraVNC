// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#ifndef _G_HIDEDESKTOP_H_
#define _G_HIDEDESKTOP_H_

// Everything

void HideDesktop();				// hide it
void RestoreDesktop();			// restore it  (warning: uses some global state set by HideDesktop)

// Just the Active Desktop

bool HideActiveDesktop();		// returns true if the Active Desktop was enabled and has been hidden
void ShowActiveDesktop();		// show it always (if Shell version >= 4.71)

// adzm - 2010-07 - Disable more effects or font smoothing
// UI Effects

void DisableEffects();
void EnableEffects();

void DisableFontSmoothing();
void EnableFontSmoothing();

#endif
