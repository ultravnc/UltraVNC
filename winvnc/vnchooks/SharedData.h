// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2001 Const Kaplinsky. All Rights Reserved.
//


// SharedData.h : Declarations for the global data in the DLL
// This file is used only when your compiler is Borland C++.

extern DWORD vnc_thread_id;
extern UINT UpdateRectMessage;
extern UINT CopyRectMessage;
extern UINT MouseMoveMessage;
extern HHOOK hCallWndHook;
extern HHOOK hGetMsgHook;
extern HHOOK hDialogMsgHook;
extern HHOOK hLLKeyboardHook;
extern HHOOK hLLMouseHook;
