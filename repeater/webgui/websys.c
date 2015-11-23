/* websys.c
 *
 * Part of the Webio Open Source lightweight web server.
 *
 * Copyright (c) 2007 by John Bartas
 * All rights reserved.
 *
 * Use license: Modified from standard BSD license.
 * 
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation, advertising 
 * materials, Web server pages, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by John Bartas. The name "John Bartas" may not be used to 
 * endorse or promote products derived from this software without 
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "websys.h"     /* port dependant system files */
#include "webio.h"

/* This file contains the routines which change from OS to OS 
 * These are:
 *
 * WI_NOBLOCKSOCK(socktype sock) - set a socket to non-blocking mode
 *
 */

#ifdef _WINSOCKAPI_
int 
WI_NOBLOCKSOCK(long sock)
{
   int   err;
   int   option = TRUE;

   err = ioctlsocket((int)sock, FIONBIO, (u_long *)&option);
   return(err);
}

/* format: "Mon, 26 Feb 2007 01:43:54 GMT" */

static char * day[] = 
{   "Sun","Mon","Tues","Wed","Thurs","Fri","Sat"};

static char * month[] = 
{  "Jan","Feb","March","April","May","June",
   "July","Aug","Sept","Oct","Nov","Dec", 
};

static char datebuf[36];

#include <time.h>

char * 
wi_getdate(wi_sess * sess)
{
   time_t      timeval;
   struct tm * gmt;

   USE_ARG(sess);
   timeval = time(NULL);
   gmt = gmtime(&timeval);

   sprintf(datebuf, "%s, %u %s %u %u:%u:%u GMT",
      day[gmt->tm_wday],
      gmt->tm_mday,
      month[gmt->tm_mon],
      gmt->tm_year + 1900, /* Windows year is based on 1900 */
      gmt->tm_hour,
      gmt->tm_min,
      gmt->tm_wday);

   return datebuf;
}

#endif /* _WINSOCKAPI_ */

#ifdef LINUX

char * 
wi_getdate(wi_sess * sess)
{

}

int
strnicmp(char * s1, char * s2, int length)
{
    int i;
    
    for(i = 0; i < length; i++)
    {
        if(((*s1++) | 0x20) != ((*s2++) | 0x20)  )
            return 1;
    }
    return 0;
}

#endif /* LINUX */

