/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


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