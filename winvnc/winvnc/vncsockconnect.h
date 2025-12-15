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


// vncSockConnect.h

// The vncSockConnect class creates a listening socket and binds
// it to the specified port. It then creates a listen thread which
// goes into a loop, listening on the socket.
// When the vncSockConnect object is destroyed, all resources are
// freed automatically, including the listen thread.

class vncSockConnect;

#if (!defined(_WINVNC_VNCSOCKCONNECT))
#define _WINVNC_VNCSOCKCONNECT

// Includes
#include "stdhdrs.h"
#include "vsocket.h"
#include <omnithread.h>

class vncServer;

// The vncSockConnect class itself
class vncSockConnect
{
public:
	// Constructor/destructor
	vncSockConnect();
	virtual~vncSockConnect();

	// Init
	virtual VBool Init(vncServer *server, UINT port);

	// Implementation
protected:
	// The listening socket
	VSocket m_socket;

	// The port to listen on
	UINT m_port;

	// The in-coming accept thread
	omni_thread *m_thread;
};

#endif // _WINVNC_VNCSOCKCONNECT