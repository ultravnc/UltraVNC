// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#pragma once

class CredentialManager {
public:
    // Read a plain VNC password from Windows Credential Manager.
    // host and port are used to build the target key: "ultravnc:vnc:<host>:<port>"
    // Returns true if a password was found and copied into outPassword (null-terminated).
    static bool ReadVncPassword(const wchar_t* host, int port,
                                char* outPassword, size_t outPasswordSize);

    // Write a plain VNC password to Windows Credential Manager.
    static bool WriteVncPassword(const wchar_t* host, int port, const char* password);

    // Delete a stored VNC password.
    static bool DeleteVncPassword(const wchar_t* host, int port);

    // Read an MS-Logon credential (domain, user, password) from Credential Manager.
    // Key: "ultravnc:mslogon:<host>:<port>"
    static bool ReadMsLogonPassword(const wchar_t* host, int port,
                                     char* outDomain, size_t domainSize,
                                     char* outUser, size_t userSize,
                                     char* outPassword, size_t passwordSize);

    // Write an MS-Logon credential to Credential Manager.
    static bool WriteMsLogonPassword(const wchar_t* host, int port,
                                     const char* domain, const char* user, const char* password);

    // Delete a stored MS-Logon credential.
    static bool DeleteMsLogonPassword(const wchar_t* host, int port);

    // Internal helper exposed for use by static helpers in CredentialManager.cpp.
    static void BuildTargetName(const wchar_t* prefix, const wchar_t* host, int port,
                                wchar_t* target, size_t targetSize);
};
