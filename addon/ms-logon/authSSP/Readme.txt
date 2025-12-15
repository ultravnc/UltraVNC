// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


Idea for AuthSSP.dll:
Authenticate user with SSPI
Impersonate this user
Check access rights against this user with the impersonation/access token.
During logon/impersonation, group membership expansion (for the token) occurs:
- Universal groups anywhere in the forest
- Global groups
- Domain local groups in the user's domain
- Local groups
- This expansion includes all nested groups

Changing the CUPG (now CUPSD) interface: No longer passing one group after the other but pass a SecurityDescriptor for Windows NT/Windows 2000/Windows XP. 
This allows for just one Windows logon attempt to check authentication and authorization.

AuthSSP.dll is only used if there's a DWORD regkey HKEY_LOCAL_MACHINE\SOFTWARE\ORL\WinVNC3\NewMSLogon set to 1.
Then all other authentication methods are skipped.