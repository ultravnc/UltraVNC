// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//
 

#include "VNCviewerApp.h"
#include "Daemon.h"

class VNCviewerApp32 : public VNCviewerApp {
public:
	VNCviewerApp32(HINSTANCE hInstance, PSTR szCmdLine);

	void NewConnection(bool Is_Listening);
	void NewConnection(bool Is_Listening,TCHAR *host, int port);
	void NewConnection(bool Is_Listening,SOCKET sock);
	void NewConnection(bool Is_Listening,TCHAR *configFile);

	~VNCviewerApp32();
private:
	// Set up registry for program's sounds
	//void RegisterSounds();
	Daemon  *m_pdaemon;
};

