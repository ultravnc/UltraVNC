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


#include "myDebug.h"
#include "stdio.h"
#include  <stdlib.h>
#include "logging.h"

void Die(const char *file,int line,const char *assertion)
{
	fprintf(stderr, "%s(%d): Internal error, assertion failed: %s\n", file,line,assertion);
	PrintLog((DEST,"Die() Called - %s(%d): Internal error, assertion failed: %s\n", file,line,assertion));
	abort();
}
