// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.

#include "stdhdrs.h"
#include "CredentialManager.h"
#include <wincred.h>
#include <wchar.h>
#pragma comment(lib, "advapi32.lib")

void CredentialManager::BuildTargetName(const wchar_t* prefix, const wchar_t* host, int port,
                                       wchar_t* target, size_t targetSize)
{
    _snwprintf_s(target, targetSize, _TRUNCATE, L"ultravnc:%s:%s:%d", prefix, host, port);
}

static void BuildVncTargetName(const wchar_t* host, int port, wchar_t* target, size_t targetSize)
{
    CredentialManager::BuildTargetName(L"vnc", host, port, target, targetSize);
}

static void BuildMsLogonTargetName(const wchar_t* host, int port, wchar_t* target, size_t targetSize)
{
    CredentialManager::BuildTargetName(L"mslogon", host, port, target, targetSize);
}

bool CredentialManager::ReadVncPassword(const wchar_t* host, int port,
                                          char* outPassword, size_t outPasswordSize)
{
    if (!host || !outPassword || outPasswordSize == 0)
        return false;

    wchar_t target[CRED_MAX_GENERIC_TARGET_NAME_LENGTH];
    BuildVncTargetName(host, port, target, _countof(target));

    PCREDENTIALW pcred = nullptr;
    if (!CredReadW(target, CRED_TYPE_GENERIC, 0, &pcred))
        return false;

    bool ok = false;
    if (pcred && pcred->CredentialBlobSize > 0 && pcred->CredentialBlob)
    {
        // CredentialBlob is a byte array; treat as ANSI string for VNC passwords.
        size_t len = pcred->CredentialBlobSize;
        if (len < outPasswordSize)
        {
            memcpy(outPassword, pcred->CredentialBlob, len);
            outPassword[len] = '\0';
            ok = true;
        }
    }

    if (pcred)
        CredFree(pcred);

    return ok;
}

bool CredentialManager::WriteVncPassword(const wchar_t* host, int port, const char* password)
{
    if (!host || !password)
        return false;

    wchar_t target[CRED_MAX_GENERIC_TARGET_NAME_LENGTH];
    BuildVncTargetName(host, port, target, _countof(target));

    size_t pwLen = strlen(password);
    if (pwLen == 0 || pwLen > 256)
        return false;

    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = target;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.CredentialBlobSize = (DWORD)pwLen;
    cred.CredentialBlob = (LPBYTE)password;
    cred.UserName = L"UltraVNC";

    return (CredWriteW(&cred, 0) != FALSE);
}

bool CredentialManager::DeleteVncPassword(const wchar_t* host, int port)
{
    if (!host)
        return false;

    wchar_t target[CRED_MAX_GENERIC_TARGET_NAME_LENGTH];
    BuildVncTargetName(host, port, target, _countof(target));

    return (CredDeleteW(target, CRED_TYPE_GENERIC, 0) != FALSE);
}

bool CredentialManager::ReadMsLogonPassword(const wchar_t* host, int port,
                                             char* outDomain, size_t domainSize,
                                             char* outUser, size_t userSize,
                                             char* outPassword, size_t passwordSize)
{
    if (!host || !outDomain || domainSize == 0 ||
        !outUser || userSize == 0 ||
        !outPassword || passwordSize == 0)
        return false;

    outDomain[0] = '\0';
    outUser[0] = '\0';
    outPassword[0] = '\0';

    wchar_t target[CRED_MAX_GENERIC_TARGET_NAME_LENGTH];
    BuildMsLogonTargetName(host, port, target, _countof(target));

    PCREDENTIALW pcred = nullptr;
    if (!CredReadW(target, CRED_TYPE_GENERIC, 0, &pcred))
        return false;

    bool ok = false;
    if (pcred && pcred->CredentialBlobSize > 0 && pcred->CredentialBlob)
    {
        // Copy password from the credential blob.
        size_t len = pcred->CredentialBlobSize;
        if (len < passwordSize)
        {
            memcpy(outPassword, pcred->CredentialBlob, len);
            outPassword[len] = '\0';
            ok = true;
        }

        // Parse domain\user from the UserName field.
        if (ok && pcred->UserName)
        {
            char userNameA[CRED_MAX_USERNAME_LENGTH];
            size_t converted = 0;
            if (wcstombs_s(&converted, userNameA, sizeof(userNameA), pcred->UserName, _TRUNCATE) == 0)
            {
                const char* sep = strchr(userNameA, '\\');
                if (sep)
                {
                    size_t dLen = sep - userNameA;
                    size_t uLen = strlen(sep + 1);
                    if (dLen < domainSize && uLen < userSize)
                    {
                        memcpy(outDomain, userNameA, dLen);
                        outDomain[dLen] = '\0';
                        memcpy(outUser, sep + 1, uLen);
                        outUser[uLen] = '\0';
                    }
                    else
                    {
                        ok = false;
                    }
                }
                else
                {
                    if (strncpy_s(outUser, userSize, userNameA, _TRUNCATE) != 0)
                        ok = false;
                }
            }
            else
            {
                ok = false;
            }
        }
    }

    if (pcred)
        CredFree(pcred);

    return ok;
}

bool CredentialManager::WriteMsLogonPassword(const wchar_t* host, int port,
                                              const char* domain, const char* user, const char* password)
{
    if (!host || !domain || !user || !password)
        return false;

    wchar_t target[CRED_MAX_GENERIC_TARGET_NAME_LENGTH];
    BuildMsLogonTargetName(host, port, target, _countof(target));

    size_t dLen = strlen(domain);
    size_t uLen = strlen(user);
    size_t pLen = strlen(password);
    if (dLen > 256 || uLen > 256 || pLen > 256)
        return false;

    // Build UserName as "domain\user" so it shows up in Credential Manager UI.
    char userNameA[CRED_MAX_USERNAME_LENGTH];
    if (dLen > 0)
        _snprintf_s(userNameA, sizeof(userNameA), _TRUNCATE, "%s\\%s", domain, user);
    else
        _snprintf_s(userNameA, sizeof(userNameA), _TRUNCATE, "%s", user);

    wchar_t userNameW[CRED_MAX_USERNAME_LENGTH];
    if (mbstowcs_s(nullptr, userNameW, _countof(userNameW), userNameA, _TRUNCATE) != 0)
        return false;

    CREDENTIALW cred = {};
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = target;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.CredentialBlobSize = (DWORD)pLen;
    cred.CredentialBlob = (LPBYTE)password;
    cred.UserName = userNameW;

    return (CredWriteW(&cred, 0) != FALSE);
}

bool CredentialManager::DeleteMsLogonPassword(const wchar_t* host, int port)
{
    if (!host)
        return false;

    wchar_t target[CRED_MAX_GENERIC_TARGET_NAME_LENGTH];
    BuildMsLogonTargetName(host, port, target, _countof(target));

    return (CredDeleteW(target, CRED_TYPE_GENERIC, 0) != FALSE);
}
