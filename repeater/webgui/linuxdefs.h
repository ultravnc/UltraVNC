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
