:: // This file is part of UltraVNC
:: // https://github.com/ultravnc/UltraVNC
:: // https://uvnc.com/
:: //
:: // SPDX-License-Identifier: GPL-3.0-or-later
:: //
:: // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
:: // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
:: //


@echo on
bcc32.exe -v- -O2 -3 -tWM -xd- -q -w-8066 -c -oomnithread.obj nt.cpp
