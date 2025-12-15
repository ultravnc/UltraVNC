// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#ifndef ACCELKEYS_H__
#define ACCELKEYS_H__

#pragma once
class AccelKeys
{
public:
	HWND   m_hWnd;
	HACCEL m_hAccelTable;
	AccelKeys();
	bool TranslateAccelKeys(MSG *pmsg);
	void SetWindowHandle(HWND hWnd) { m_hWnd = hWnd; }
	virtual ~AccelKeys();	
};

#endif 
