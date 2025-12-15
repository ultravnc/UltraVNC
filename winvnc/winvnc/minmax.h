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


// Routines to calculate the maximum and minimum of two integers

#if !defined(MINMAX_INCLUDED)
#define MINMAX_INCLUDED
#pragma once

// Some routines used internally to minimise and maximise integers
static inline int Max(int x, int y)
{
	if (x>y)
		return x;
	else
		return y;
}

static inline int Min(int x, int y)
{
	if (x>y)
		return y;
	else
		return x;
}

#endif