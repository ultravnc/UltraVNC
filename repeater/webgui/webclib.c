/* webclib.c
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
 */

#include "websys.h"     /* port dependant system files */
#include "webio.h"
#include "webfs.h"


static char output[DDB_SIZE];
int   ssi_threshhold = DDB_SIZE/2;

void
wi_printf(wi_sess * sess, char * fmt, ...)
{
   int   len;
   va_list a;

   /* Since it's a huge pain to check the connection after each CGI write,
    * we may get sometimes handed a dying connection. Ignore these. 
    */
   if(sess->ws_state == WI_ENDING)
      return;

   /* Try to make sure we won't overflow the print buffer */
   len = strlen(fmt);
   if(len > sizeof(output)/2 )
   {
      dtrap();
      dprintf("wi_printf: overflow, fmt: %s\n", fmt);
      return;
   }

   va_start(a, fmt);
   vsprintf(output, fmt, a);
   va_end(a);

   /* See if we overflowed the print buffer */
   len = strlen(output);
   if((output[DDB_SIZE-1] != 0) || len >= sizeof(output))
   {
      dprintf("wi_printf: overflow, output: %s\n", output);
      panic("wi_printf");
	  return;
   }

   /* Print warnings if we even came close. */
   if(len > sizeof(output)/2 )
   {
      dtrap();
      dprintf("wi_printf warning: oversize line: %s", output);
   }

   /* see if new data will fit in existing buffer */
   if((sess->ws_txtail == NULL) || 
      (len >= (WI_TXBUFSIZE - sess->ws_txtail->tb_total)))
   {
      /* won't fit, get another buffer */
      if(wi_txalloc(sess) == NULL)
         return;
   }

   MEMCPY( &sess->ws_txtail->tb_data[sess->ws_txtail->tb_total],
      output, len);
   sess->ws_txtail->tb_total += len;
   return;
}


int
wi_putlong(wi_sess * sess, u_long value)
{
   wi_printf(sess, "%lu", value);
   return 0;
}

int
wi_putstring(wi_sess * sess, char * string)
{
   wi_printf(sess, "%s", string);
   return 0;
}

/* wi_formvalue() - get a value from a form. form should be in the 
 * ws_formlist of the session passed. name of form control for 
 * which to get the value is also passed.
 *
 * Returns prt to value string in formif name is found,
 * returns NULL if name is not found.
 */

char *
wi_formvalue( wi_sess * sess, char * ctlname )
{
   int      i;
   int      namelen;
   struct wi_form_s * form;

   namelen = strlen(ctlname);
   for(form = sess->ws_formlist; form; form = form->next)
   {
      for(i = 0; i < form->paircount; i++)
         if( _strnicmp(form->pairs[i].name, ctlname, namelen) == 0 )
         {
            if( (*form->pairs[i].value) == 0)
               return NULL;
            else
               return form->pairs[i].value;
         }
   }

   return NULL;
}


/* wi_checkip() - helper for wi_formipaddr() */

char *
wi_checkip(u_long * out, char * input)
{
    int         octet;  /* Counter, 1-4 octets */
    unsigned    value;
    char *      cp;
    u_long      ipval;

    ipval = 0;
    cp = input;

    /* Note - try for 5 values; however finding more than 4 
     * is error. Correct loop exit if via "!cp".
     */
    for(octet = 1; octet < 5; octet++)
    {
        value = atoi(cp);
        if((value == 0) && (*cp != '0'))
            return ("All chars must be digits or dots");

        if(value > 255)
            return ("All values must be 0-255");

        ipval = (ipval << 8) + value;
        cp = strchr(cp, '.');
        if(!cp)
            break;
        cp++;
    }

    if((octet > 4) || (octet < 2))
        return ("Must be 1 to 3 dots");

    /* If there are missing ocets (e.g. "10.1") then shift the high and low 
     * values to allow the traditional shorthand.
     */
    if(octet == 2)
    {
        ipval = (ipval & 0x000000FF) + ((ipval & 0x0000FF00) << 16);
    }
    else if(octet == 3)
    {
        ipval = (ipval & 0x000000FF) + ((ipval & 0x00FFFF00) << 8);
    }

    *out = htonl(ipval);
    return NULL;        /* No error */
}

/* wi_formipaddr()
 *
 * Extract a form address from name/value pairs attached to the session. 
 *
 * Returns NULL if IP was parsed and placed in ipaddr, else returns
 * some text describng the problem.
 */

char *
wi_formipaddr( wi_sess * sess, char * ipname, u_long * ipaddr)
{
   char * iptext = wi_formvalue(sess, ipname );

   if (!iptext)
      return "unknown form control name";

   return (wi_checkip(ipaddr, iptext));
}

/* wi_formipaddr()
 *
 * Extract an integer from name/value pairs attached to the session. 
 *
 * Returns 0 if int was parsed and placed in return_int, else returns
 * negative ENP_ error.
 */

int
wi_formint(wi_sess * sess, char * name, long * return_int )
{
   char * valuetext;

   valuetext = wi_formvalue( sess, name );
   if(valuetext == NULL)
      return WIE_BADPARM;

   *return_int = (long)atol(valuetext);
   if( (*return_int == 0) && (*valuetext != '0'))
      return WIE_BADPARM;
   else
      return 0;
}


int
wi_formbool(wi_sess * sess, char * name)
{
   char * valuetext;

   valuetext = wi_formvalue( sess, name );
   if(valuetext == NULL)
   {
      return 0;   /* Default: FALSE */
   }

   if( (((*valuetext) & 0x20) == 'y') ||      /* Yes */
       (((*valuetext) & 0x20) == 't') ||      /* True */
         (*valuetext == 'c') )                /* checked */
   {
      return TRUE;
   }
   if( _stricmp(valuetext, "on") == 0)      /* on */
      return TRUE;

   return FALSE;
}


