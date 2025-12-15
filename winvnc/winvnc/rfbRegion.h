// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002-2003 RealVNC Ltd. All Rights Reserved.
//


//#define USE_X11_REGIONS

#ifdef USE_X11_REGIONS
#include "rfbRegion_X11.h"
#else

#include "rfbRegion_win32.h"


#endif // X11
