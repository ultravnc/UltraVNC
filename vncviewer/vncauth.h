/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2013 UltraVNC Team Members. All Rights Reserved.
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
// If the source code for the program is not available from the place from
// which you received this file, check 
// http://www.uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


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

