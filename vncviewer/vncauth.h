// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


/* 
 * vncauth.h - describes the functions provided by the vncauth library.
 */

#define MAXPWLEN 8
#define CHALLENGESIZE 16
#define CHALLENGESIZEMS 64

void vncEncryptBytes(unsigned char *bytes, char *passwd);
void vncEncryptPasswd(unsigned char *encryptedPasswd, char *passwd);
void vncEncryptPasswdMs(unsigned char *encryptedPasswd, char *passwd);
char *vncDecryptPasswd(const unsigned char *encryptedPasswd);
char *vncDecryptPasswdMs(const unsigned char *encryptedPasswd);
// marscha@2006
void vncEncryptBytes2(unsigned char *bytes, const int length, unsigned char *key);

