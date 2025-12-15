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


// vncHTTPConnect.h

// The vncHTTPConnect class creates a listening socket and binds
// it to the specified port number. It then creates a listen
// thread which goes into a loop, listening on the socket.
// When the vncHTTPConnect object is destroyed, all resources are
// freed automatically, including the listen thread.
// This server allows clients to request the java classes required
// to view the desktop remotely.

class vncHTTPConnect;

#if (!defined(_WINVNC_VNCHTTPCONNECT))
#define _WINVNC_VNCHTTPCONNECT

// Includes
#include "stdhdrs.h"
#include "vsocket.h"
#include <omnithread.h>

class vncServer;

// The vncHTTPConnect class itself
class vncHTTPConnect
{
public:
	// Constructor/destructor
	vncHTTPConnect();
	virtual ~vncHTTPConnect();

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

#endif // _WINVNC_VNCHTTPCONNECT