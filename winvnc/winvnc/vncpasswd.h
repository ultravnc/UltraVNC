// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//


// vncPasswd

// This header provides helpers for handling encrypted password data.
// The password handling routines found in vncauth.h should not be used directly

class vncPasswd;

#if (!defined(_WINVNC_VNCPASSWD))
#define _WINVNC_VNCPASSWD

#include "stdhdrs.h"
#ifdef _MSC_VER
extern "C" {
#include "vncauth.h"
}
#else
#include "vncauth.h"
#endif

// Helper to test whether an encrypted password buffer is empty/unset.
// Do not use strlen() on the 8-byte encrypted value: DES can produce a NUL
// byte as the first ciphertext byte, which would be mistaken for an empty
// C string.
inline bool vncPasswdIsEmpty(const char encrypted[MAXPWLEN])
{
    for (int i = 0; i < MAXPWLEN; ++i) {
        if (encrypted[i] != 0)
            return false;
    }
    return true;
}

// Password handling helper class
class vncPasswd
{
public:

    // Password decryptor!
    class ToText
    {
    public:
	inline ToText(const char encrypted[MAXPWLEN], bool secure)
	{
	    //vnclog.Print(LL_INTINFO, VNCLOG("PASSWD : ToText called\n"));
		char* emptyPasswd = (char*)malloc(1);
		emptyPasswd[0] = 0;
	    plaintext = vncPasswdIsEmpty(encrypted)
				? emptyPasswd
				: vncDecryptPasswd((char*)encrypted, secure);

	}
	inline ~ToText()
	{
	    if (plaintext != NULL)
	    {
		ZeroMemory(plaintext, strlen(plaintext));
		free(plaintext);
	    }
	}
	inline operator const char*() const {return plaintext;};
    private:
	char *plaintext;
    };

    class FromText
    {
    public:
	inline FromText(char *unencrypted, bool secure)
	{
	    vnclog.Print(LL_INTINFO, VNCLOG("PASSWD : FromText called\n"));
	    vncEncryptPasswd(unencrypted, encrypted, secure);
	    // ZeroMemory(unencrypted, strlen(unencrypted));
	}
	inline ~FromText()
	{
	}
	inline operator const char*() const {return encrypted;};
    private:
	char encrypted[MAXPWLEN];
    };

    class FromClear
    {
    public:
	inline FromClear(bool secure)
	{
	    //vnclog.Print(LL_INTINFO, VNCLOG("PASSWD : FromClear called\n"));
	    vncEncryptPasswd((char*)"", encrypted, secure);
	}
	inline ~FromClear()
	{
	}
	inline operator const char*() const {return encrypted;};
    private:
	char encrypted[MAXPWLEN];
    };
};

#endif