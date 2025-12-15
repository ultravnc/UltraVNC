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


#ifndef _MYDEBUG_H
#define _MYDEBUG_H

//Assert macro
#define My_assert(e)	((e) ? (void)0 : Die(__FILE__, __LINE__, #e))

//Function to halt program execution
void Die(const char *file,int line,const char *assertion);

#endif