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


/* linuxdefs.h
 *
 * Definitions for Linux port of webio demo.
 */

#ifndef _LINUXDEFS_H_
#define _LINUXDEFS_H_    1

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <malloc.h>

#define socketclose(s) close(s)

typedef unsigned long u_long;

void * memset( void *, int chr, size_t length);
#define stricmp(s,t)  strcasecmp(s,t)

#define WI_NOBLOCKSOCK(socket) fcntl(socket, F_SETFL, O_NONBLOCK)

#define closesocket(socket) close(socket)

#endif /* LINUXDEFS */
