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
 * vncauth.c - Functions for VNC password management and authentication.
 */

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
*/

#include "stdhdrs.h"
#include "vncauth.h"
#include "../common/d3des.h"


/*
 * We use a fixed key to store passwords, since we assume that our local
 * file system is secure but nonetheless don't want to store passwords
 * as plaintext.
 */

unsigned char fixedkey[8] = {23,82,107,6,35,78,88,7};


/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */

void
vncEncryptBytes(unsigned char *bytes, char *passwd)
{
    unsigned char key[8];
    unsigned int i;

    /* key is simply password padded with nulls */

    for (i = 0; i < 8; i++) {
	if (i < strlen(passwd)) {
	    key[i] = passwd[i];
	} else {
	    key[i] = 0;
	}
    }

    deskey(key, EN0);

    for (i = 0; i < CHALLENGESIZE; i += 8) {
		des(bytes+i, bytes+i);
    }
}


/*
 * Encrypt a password into the specified space.
 * encryptedPasswd will be 8 bytes long - sufficient space 
 *   should be allocated.
 */

void
vncEncryptPasswd( unsigned char *encryptedPasswd, char *passwd )
{
	unsigned int i;

    /* pad password with nulls */
    for (i = 0; i < MAXPWLEN; i++) {
		if (i < strlen(passwd)) {
			encryptedPasswd[i] = passwd[i];
		} else {	
			encryptedPasswd[i] = 0;
		}
    }

    /* Do encryption in-place - this way we overwrite our copy of the plaintext
       password */
    deskey(fixedkey, EN0);
    des(encryptedPasswd, encryptedPasswd);
}

void
vncEncryptPasswdMs( unsigned char *encryptedPasswd, char *passwd )
{
	unsigned int i;

    /* pad password with nulls */
    for (i = 0; i < 32; i++) {
		if (i < strlen(passwd)) {
			encryptedPasswd[i] = passwd[i];
		} else {	
			encryptedPasswd[i] = 0;
		}
    }

    /* Do encryption in-place - this way we overwrite our copy of the plaintext
       password */
    deskey(fixedkey, EN0);
    des(encryptedPasswd, encryptedPasswd);
}


/*
 * Decrypt a password. Returns a pointer to a newly allocated
 * string containing the password or a null pointer if the password could
 * not be retrieved for some reason.
 */

char *
vncDecryptPasswd(const unsigned char *encryptedPasswd)
{
    unsigned int i;
    unsigned char *passwd = (unsigned char *)malloc(MAXPWLEN+1);

	memcpy(passwd, encryptedPasswd, MAXPWLEN);

    for (i = 0; i < MAXPWLEN; i++) {
		passwd[i] = encryptedPasswd[i];
    }

    deskey(fixedkey, DE1);
    des(passwd, passwd);

    passwd[MAXPWLEN] = 0;

    return (char *)passwd;
}

char *
vncDecryptPasswdMs(const unsigned char *encryptedPasswd)
{
    unsigned int i;
    unsigned char *passwd = (unsigned char *)malloc(32+1);

	memcpy(passwd, encryptedPasswd, 32);

    for (i = 0; i < 32; i++) {
		passwd[i] = encryptedPasswd[i];
    }

    deskey(fixedkey, DE1);
    des(passwd, passwd);

    passwd[32] = 0;

    return (char *)passwd;
}


/*
 *   marscha@2006
 *   Encrypt bytes[length] in memory using key.
 *   Key has to be 8 bytes, length a multiple of 8 bytes.
*/
void
vncEncryptBytes2(unsigned char *where, const int length, unsigned char *key) {
	int i, j;
	deskey(key, EN0);
	for (i = 0; i< 8; i++)
		where[i] ^= key[i];
	des(where, where);
	for (i = 8; i < length; i += 8) {
		for (j = 0; j < 8; j++)
			where[i + j] ^= where[i + j - 8];
		des(where + i, where + i);
	}
}
