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


/* 
 * vncauth.h - describes the functions provided by the vncauth library.
 */

#define MAXPWLEN 8
#define MAXMSPWLEN 32
#define CHALLENGESIZE 16
#define CHALLENGESIZEMS 64

#ifdef __cplusplus
extern "C"
{
#endif
extern int vncEncryptPasswd(char *passwd, char *fname, int secure);
extern char *vncDecryptPasswd(char *fname, int secure);
extern void vncRandomBytes(unsigned char *bytes);
extern void vncRandomBytesMs(unsigned char *bytes);
extern void vncEncryptBytes(unsigned char *bytes, const char *passwd);
extern void vncSetDynKey(unsigned char key[8]);
// marscha
extern void vncDecryptBytes(unsigned char *bytes, const int length, const unsigned char *key);
#ifdef __cplusplus
}
#endif
