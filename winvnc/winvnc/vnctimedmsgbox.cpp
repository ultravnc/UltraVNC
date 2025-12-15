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


// vncTimedMsgBox.cpp

// vncTimedMsgBox::Do spawns an omni-thread to draw the message
// box and wait a few seconds before returning, leaving the message-box displayed
// until UltraVNC Server quits.

#include "stdhdrs.h"
#include "omnithread.h"

#include "vnctimedmsgbox.h"

// The message-box delay
const UINT TIMED_MSGBOX_DELAY = 1000;

// The vncTimedMsgBoxThread class

class vncTimedMsgBoxThread : public omni_thread
{
public:
	vncTimedMsgBoxThread(const char *caption, const char *title, UINT type)
	{
		m_type = type;
		m_caption = _strdup(caption);
		m_title = _strdup(title);
	};
	virtual ~vncTimedMsgBoxThread()
	{
		if (m_caption != NULL)
			free(m_caption);
		if (m_title != NULL)
			free(m_title);
	};
	virtual void run(void *)
	{
		// Create the desired dialog box
		if (m_caption == NULL)
			return;
		MessageBoxSecure(NULL, m_caption, m_title, m_type | MB_OK|MB_SETFOREGROUND|MB_SERVICE_NOTIFICATION);
	};
	char *m_caption;
	char *m_title;
	UINT m_type;
};

// The main vncTimedMsgBox class

void
vncTimedMsgBox::Do(const char *caption, const char *title, UINT type)
{
	// Create the thread object
	vncTimedMsgBoxThread *thread = new vncTimedMsgBoxThread(caption, title, type);
	if (thread == NULL)
		return;

	// Start the thread object
	thread->start();

	// And wait a few seconds
	Sleep(TIMED_MSGBOX_DELAY);
}