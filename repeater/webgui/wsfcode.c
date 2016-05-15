/* wsfcode.c
 * 
 * This file was created by an automated utility. 
 * It is not intended for manual editing
 */

#include "websys.h"
#include "webio.h"
#include "webfs.h"
#include "wsfdata.h"



/* SSI
 *
 * memory_ssi routine stub
 */

int
memory_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * mode1_ssi routine stub
 */

int
mode1_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * mode2_ssi routine stub
 */

int
mode2_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * sport_ssi routine stub
 */

int
sport_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * vport_ssi routine stub
 */

int
vport_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * acon_ssi routine stub
 */

int
acon_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * rcon_ssi routine stub
 */

int
rcon_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * aid_ssi routine stub
 */

int
aid_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * acons_ssi routine stub
 */

int
acons_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * rcons_ssi routine stub
 */

int
rcons_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * aids_ssi routine stub
 */

int
aids_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * webport_ssi routine stub
 */

int
webport_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * log_ssi routine stub
 */

int
log_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * ucom_ssi routine stub
 */

int
ucom_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * listcomment_ssi routine stub
 */

int
listcomment_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * connections_ssi routine stub
 */

int
connections_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * server_access_ssi routine stub
 */

int
server_access_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * viewer_access_ssi routine stub
 */

int
viewer_access_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* SSI
 *
 * keepalive_ssi routine stub
 */

int
keepalive_ssi(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}



/* PUSH
 *
 * pushtest_func routine stub
 */

int
pushtest_func(wi_sess * sess, EOFILE * eofile)
{
   /* Add your code here */
   return 0;
}


/* testaction_cgi
 *
 * Stub routine for form processing
 * 
 * Returns NULL if OK, else error message.
 */

char *
testaction_cgi(wi_sess * sess)
{
   char *   hidden;
   char *   ucom_on;
   char *   web_port;
   char *   id_con;
   char *   id_on;
   char *   refuse_con;
   char *   refuse_on;
   char *   allow_con;
   char *   allow_on;
   char *   viewer_port;
   char *   server_port;
   char *   mode2;
   char *   mode1;
   char *   keepalive;

   hidden = wi_formvalue(sess, "hidden");   /* default:  */
   ucom_on = wi_formvalue(sess, "ucom_on");   /* default: ucom_on */
   web_port = wi_formvalue(sess, "web_port");   /* default:  */
   id_con = wi_formvalue(sess, "id_con");   /* default:  */
   id_on = wi_formvalue(sess, "id_on");   /* default: id_on */
   refuse_con = wi_formvalue(sess, "refuse_con");   /* default:  */
   refuse_on = wi_formvalue(sess, "refuse_on");   /* default: refuse_on */
   allow_con = wi_formvalue(sess, "allow_con");   /* default:  */
   allow_on = wi_formvalue(sess, "allow_on");   /* default: allow_on */
   viewer_port = wi_formvalue(sess, "viewer_port");   /* default:  */
   server_port = wi_formvalue(sess, "server_port");   /* default:  */
   mode2 = wi_formvalue(sess, "mode2");   /* default: mode2 */
   mode1 = wi_formvalue(sess, "mode1");   /* default: mode1 */
   keepalive = wi_formvalue(sess, "keepalive");   /* default: keepalive */


   return NULL;
}


/* testaction2_cgi
 *
 * Stub routine for form processing
 * 
 * Returns NULL if OK, else error message.
 */

char *
testaction2_cgi(wi_sess * sess)
{
   char *   hidden;
   char *   comment;
   char *   id;

   hidden = wi_formvalue(sess, "hidden");   /* default:  */
   comment = wi_formvalue(sess, "comment");   /* default:  */
   id = wi_formvalue(sess, "id");   /* default:  */


   return NULL;
}


/* testaction3_cgi
 *
 * Stub routine for form processing
 * 
 * Returns NULL if OK, else error message.
 */

char *
testaction3_cgi(wi_sess * sess)
{
   char *   hidden;

   hidden = wi_formvalue(sess, "hidden");   /* default:  */


   return NULL;
}


/* testaction4_cgi
 *
 * Stub routine for form processing
 * 
 * Returns NULL if OK, else error message.
 */

char *
testaction4_cgi(wi_sess * sess)
{
   char *   hidden;
   char *   id;

   hidden = wi_formvalue(sess, "hidden");   /* default:  */
   id = wi_formvalue(sess, "id");   /* default:  */


   return NULL;
}


/* testaction5_cgi
 *
 * Stub routine for form processing
 * 
 * Returns NULL if OK, else error message.
 */

char *
testaction5_cgi(wi_sess * sess)
{
   char *   hidden;

   hidden = wi_formvalue(sess, "hidden");   /* default:  */


   return NULL;
}


/* passwd_cgi
 *
 * Stub routine for form processing
 * 
 * Returns NULL if OK, else error message.
 */

char *
passwd_cgi(wi_sess * sess)
{
   char *   hidden;
   char *   newpass;
   char *   oldpass;

   hidden = wi_formvalue(sess, "hidden");   /* default:  */
   newpass = wi_formvalue(sess, "newpass");   /* default:  */
   oldpass = wi_formvalue(sess, "oldpass");   /* default:  */


   return NULL;
}


int
wi_cvariables(wi_sess * sess, int token)
{
   int   e;

   switch(token)
   {
   case MEMHITS_VAR31:
      e = wi_putlong(sess, (u_long)(wi_totalblocks));
      break;
   }
   return e;
}

