/* webio.c
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


#include "websys.h"     /* port dependent system files */
#include "webio.h"
#include "webfs.h"

/* This file contains the main entry points for the webio library */



/* The web server "listen" socket */ 
socktype wi_listen=INVALID_SOCKET;

socktype wi_highsocket=INVALID_SOCKET;

/* Port number on which to listen. May be changed prior to calling webinit */

char * wi_rootfile = "index.html";  /* File name to substitute for "/" */
extern int saved_portHTTP;

/* Flag to permit connections by the localhost only (security). */
int   wi_localhost;

#ifdef WI_THREAD
struct timeval   wi_seltmo = {5,0}; /* on thread, short block is OK */
#else
struct timeval   wi_seltmo = {0,0}; /* polled mode - no blocking */
#endif

/* webinit()
 * 
 * This should be the first call made to the web server. It initializes
 * the embedded file system starts a listen
 * 
 * Parameters:
 * int stdfiles - flag to use system "fopen" file calls as well as embedded FS
 * 
 * Returns 0 if OK, else negative error code.
 */

int
wi_init()
{
   struct sockaddr_in   wi_sin;
   int      error;

   /* Create the web server "listen" socket */
   wi_listen = socket(AF_INET, SOCK_STREAM, 0);
   if(wi_listen == INVALID_SOCKET)
   {
      dprintf("Error open socket for listen\n");
      return WIE_SOCKET;
   }

   wi_sin.sin_family = AF_INET;
   wi_sin.sin_addr.s_addr = htonl(INADDR_ANY);
   wi_sin.sin_port = htons( saved_portHTTP);
   error = bind(wi_listen, (struct sockaddr*)&wi_sin, 
      sizeof(struct sockaddr_in));
   if(error)
   {
	  closesocket(wi_listen);
      dprintf("Error %d binding web server\n", error);
      return WIE_SOCKET;
   }

   error = listen(wi_listen, 15);
   if (error)
   {
	  closesocket(wi_listen);
      dprintf("Error %d starting listen\n", error);
      return WIE_SOCKET;
   }   
   return 0;
}


/* webpoll() - entry point for driving webio in a "polled" manner.
 * this checks for any work that needs to be done and returns. It
 * may be preempted, but is not re-entrant.
 * 
 * Returns negative code on error, else number of open sessions.
 * Return of 0 means no sessions and no error.
 */


int
wi_poll()
{
   wi_sess * sess;
   wi_sess * next_sess;
   int   sessions = 0;
   int   recvs;
   int   sends;
   int   error;
   fd_set sel_recv;
   fd_set sel_send;
   fd_set sel_except;
   char * data;

   memset(&sel_recv, 0, sizeof(sel_recv));
   memset(&sel_send, 0, sizeof(sel_send));
   memset(&sel_except, 0, sizeof(sel_except));

   /* add listen sock to select list */
   FD_SET(wi_listen, &sel_recv);
   wi_highsocket = wi_listen;

   /* loop through list of open sessions looking for work */
   recvs = sends = 0;
   for(sess = wi_sessions; sess; sess = sess->ws_next)
   {
      if(sess->ws_socket == INVALID_SOCKET)
         continue;

	  FD_SET(sess->ws_socket, &sel_except);

      /* If socket is reading, load for a select */
      if(sess->ws_state == WI_HEADER)
      {
         recvs++;
         FD_SET(sess->ws_socket, &sel_recv);
         if(sess->ws_socket > wi_highsocket)
             wi_highsocket = sess->ws_socket;
      }

      if((sess->ws_txbufs) ||
         (sess->ws_flags & WF_BINARY))
      {
         sends++;
         FD_SET(sess->ws_socket, &sel_send);
         if(sess->ws_socket > wi_highsocket)
             wi_highsocket = sess->ws_socket;
      }
   }
   wi_highsocket++;     /* Select mumbo-jumbo */

   /* See if any of the sockets have input or ready to send */
   sessions = select( wi_highsocket, &sel_recv, &sel_send, &sel_except, &wi_seltmo);
   if(sessions == SOCKET_ERROR)
   {
      error = errno;
      dprintf("select error %d\n", error );
	  sess = wi_sessions; 
		while(sess)
			{
				next_sess = sess->ws_next;
				if(FD_ISSET(sess->ws_socket, &sel_except))
					{
						wi_delsess(sess);
						return WIE_SOCKET;
					}
				sess = next_sess;
			}
      return WIE_SOCKET;
   }

   /* see if we have a new connection request */
   if(FD_ISSET(wi_listen, &sel_recv))
   {
      error = wi_sockaccept();
      if(error)
      {
         printf("Socket accept error %d\n", error);
         return error;
      }
   }

   sess = wi_sessions; 
   while(sess)
   {
      next_sess = sess->ws_next;

      /* jump to here to accelerate things if a session changes state */
another_state:    

      switch(sess->ws_state)
      {
      case WI_HEADER:
         /* See if there is data to read */
         if(FD_ISSET(sess->ws_socket, &sel_recv))
         {
            error = recv(sess->ws_socket, 
               sess->ws_rxbuf + sess->ws_rxsize,
               sizeof(sess->ws_rxbuf) - sess->ws_rxsize, 0);
			//if FD_ISSET=true then there is date unless remote closed connection
			// recv()==0 indicate a remote session taht's closed.
            if(error <= 0)
            {
               error = errno;
               dprintf("sock recv error %d\n", error );
			   wi_delsess(sess);
               return WIE_SOCKET;
            }
            sess->ws_rxsize += error;
            sess->ws_last = cticks;
         }
		 if(sess->ws_rxsize>=WI_RXBUFSIZE)
		 {
			 //problem, buffer is to small and we keep looping
			 sess->ws_rxsize=1;
			 wi_delsess(sess);
			 return WIE_SOCKET;
		 }
         if(sess->ws_rxsize)  /* unprocessed input http */
         {
            error = wi_parseheader( sess );  /* Make a best effort to process input */
            sessions++;
         }
         /* If the logic above pushed session into POSTRX (waiting for POST 
          * operation) jump to POSTRX logic, else break.
          */
         if(sess->ws_state != WI_HEADER)
            goto another_state;
         break;

      case WI_POSTRX:
         /* See if there is more to read */
         error = recv(sess->ws_socket, 
            sess->ws_rxbuf + sess->ws_rxsize,
            sizeof(sess->ws_rxbuf) - sess->ws_rxsize, 0);

         if(error < 0)
         {
            if(errno == EWOULDBLOCK)
               error = 0;
            else
            {
               error = errno;
               dprintf("sock recv error %d\n", error );
			   wi_delsess(sess);
               return WIE_SOCKET;
            }
         }
         sess->ws_rxsize += error;
         sess->ws_last = cticks;

         /* If we have all the content, parse the name/value pairs. 
          * We check for ContentLength field or socket closed 
          */
         data = sess->ws_data;
         if(data)
         {
            int   contentRx;

            contentRx = sess->ws_rxsize - (data - sess->ws_rxbuf);

            if((contentRx >= sess->ws_contentLength) ||
               (error == ENOTCONN))
            {
               error = wi_buildform(sess, data);
               if(error)
               {
                  wi_senderr(sess, 400);  /* Bad request */
                  break;
               }
               sess->ws_state = WI_CONTENT;
               sess->ws_last = cticks;
            }
         }
         if(sess->ws_state != WI_POSTRX)
            goto another_state;

         break;

      case WI_CONTENT:
         error = wi_readfile(sess);
         if(error)
         {
            sess->ws_state = WI_ENDING;
         }
         sessions++;
         if(sess->ws_state != WI_CONTENT)
            goto another_state;

         break;

      case WI_SENDDATA:
         if((sess->ws_txbufs || (sess->ws_flags & WF_BINARY)) && 
            FD_ISSET(sess->ws_socket, &sel_send))
         {
            /* socket has data to write */
            error = wi_sockwrite(sess);
            if(error)
            {
               sess->ws_state = WI_ENDING;
            }
            sessions++;
         }
         if(sess->ws_state != WI_SENDDATA)
            goto another_state;
         break;
      case WI_ENDING:
         /* Don't delete session and break, else we'll get a fault
          * in the sess->ws_last test below.
          */
         wi_delsess(sess);
         sess = next_sess;
         continue;
      default:
         dtrap();
         break;
      }
      /* kill sessions with no recent activity */
      if((u_long)(sess->ws_last + (15 * TPS)) < cticks)
      {
         dtrap();
         dprintf("killing stuck webio session\n");
         wi_delsess(sess);
      }

      sess = next_sess;
   }

   return sessions;
}


int
wi_sockaccept()
{
   struct sockaddr_in sa;
   socktype    newsock;
   wi_sess *   newsess;
   int         sasize;
   int         error;

   sasize = sizeof(struct sockaddr_in);
   newsock = accept(wi_listen, (struct sockaddr * )&sa, &sasize);
   if (newsock==-1)
   {
	   DWORD aa= GetLastError();
	   int bb=1;
   }
   if(sasize != sizeof(struct sockaddr_in))
   {
      dtrap();
      return WIE_SOCKET;
   }

   /* If the localhost-only flag is set, reject all other hosts */
   if(wi_localhost)
   {
      /* see if remote host is 127.0.0.1 or other version of self */
      if(htonl(sa.sin_addr.s_addr) != 0x7F000001)
      {
         struct sockaddr_in local;
         int slen;

         /* wasn't loopback, check for local IP.
   	    * First get socket's local IP
          */
         memset(&local, 0, sizeof(local));
         slen = sizeof(local);
         if( getsockname(newsock, (struct sockaddr *)&local, &slen) < 0) 
         {
             closesocket(newsock);
             return -1;
         }

         if(sa.sin_addr.s_addr != local.sin_addr.s_addr)
         {
            closesocket(newsock);
            return 0;   /* not an error */
         }
      }
   }

   /* Set every socket to non-blocking. */
   error = WI_NOBLOCKSOCK(newsock);
   if(error)
   {
      dtrap();
      panic("blocking socket");
	  return WIE_SOCKET;
   }

   /* now that we have a new socket connection, make a session 
    * object for it 
    */
   newsess = wi_newsess();
   if(!newsess)
      return WIE_MEMORY;
   newsess->ws_socket = newsock;
      
   return 0;
}


/* wi_parseheader()
 *
 * Make a best effort to process input. This is most often an http 
 * header command (e.g. "GET foo.html"). This may be called with 
 * only a partial header in sess->ws_rxbuf, or when the socket 
 * is already in use for a previous command on a persistent 
 * connention. 
 *
 * Returns: 0 if no error (incomplete header is not an error) 
 * else negative error code.
 */

int
wi_parseheader( wi_sess * sess )
{
   char *   cp;
   char *   cl;
   char *   rxend;
   char *   pairs;
   u_long   cmd;
   int      error;
   int i;
   	char *strs[38] = {"index.html","passwd.html","settings.html","ok.html","nok.html","log.html","comment.html","images2.jpg",
	 "connections.txt","viewer_access.txt","server_access.txt","memory.ssi","mode1.ssi","mode2.ssi","sport.ssi",
	 "vport.ssi","acon.ssi","rcon.ssi","aid.ssi","acons.ssi","rcons.ssi","aids.ssi","webport.ssi","log.ssi","ucom.ssi",
	 "listcomment.ssi","connections.ssi","server_access.ssi","viewer_access.ssi","keepalive.ssi","memhits.var","pushtest.htm",
	 "testaction.cgi","testaction2.cgi","testaction3.cgi","testaction4.cgi","testaction5.cgi","passwd.cgi"};

   /* First find end of HTTP header */
   rxend = strstr(sess->ws_rxbuf, "\r\n\r\n" );
   if(!rxend)
      return 0;   /* no header yet - wait some more */

   sess->ws_data = rxend + 4;

   /* extract the basic http comand */
   cmd = sess->ws_rxbuf[0];
   cmd <<= 8;
   cmd |= sess->ws_rxbuf[1];
   cmd <<= 8;
   cmd |= sess->ws_rxbuf[2];
   cmd <<= 8;
   cmd |= sess->ws_rxbuf[3];

   switch(cmd)
   {
   case H_GET:
   case H_POST:
      sess->ws_cmd = (httpcmds)cmd;
      break;
   case H_PUT:
      /* Deal with PUT operations in another path */
      if(cmd == H_PUT)
         return ( wi_putfile(sess) );
   default:
      dtrap();
      /* unsupported command - send eror and clean up */
      wi_senderr(sess, 501);
      sess->ws_flags &= ~WF_READINGCMDS;
      sess->ws_state = WI_ENDING;
      return -1;
   }


   /* Fall to here for GET or POST. Extract the URL */
   cp = wi_nextarg(&sess->ws_rxbuf[3]);
   if(!cp)
   {
      wi_senderr(sess, 400);  /* Bad request */
      return WIE_CLIENT;
   }
   if(*cp == '/')
   {
      if(*(cp+1) == ' ')
         sess->ws_uri = wi_rootfile;
      else
         sess->ws_uri = cp+1;    /* strip leading slash */
   }
   else
      sess->ws_uri = cp;

   /* Extract other useful fields from header  */
   sess->ws_auth = wi_getline("Authorization:", cp);
   sess->ws_referer = wi_getline("Referer:", cp);
   sess->ws_host = wi_getline("Host:", cp);

   cl = wi_getline("Content-Length:", cp);
   if(cl)
      sess->ws_contentLength = atoi(cl);
   else
      sess->ws_contentLength = 0;  /* unset */

   /* Check for name/value pairs and build form if found */
   if(cmd == H_GET)
   {
      pairs = strchr(cp, '?');
      if(pairs)
      {
         *pairs++ = 0;     /* Null terminate URI field */
         error = wi_buildform(sess, pairs);
         if(error)
         {
            wi_senderr(sess, 400);  /* Bad request */
            return WIE_CLIENT;
         }
      }
   }
   else if(cmd != H_GET) /* POST command */
   {
      if(cmd != H_POST)
      {
         wi_senderr(sess, 400);  /* Bad request */
         return WIE_CLIENT;
      }
      /* fall to header parse logic, get name/values from body later */
   }

   /* insert the null terminators in any strings in the rxbuf */
   if((sess->ws_uri > sess->ws_rxbuf) && (sess->ws_uri < rxend))
      if (sess->ws_uri)  wi_argterm(sess->ws_uri);      /* Null terminate the URI */
   if((sess->ws_auth > sess->ws_rxbuf) && (sess->ws_auth < rxend))
      if (sess->ws_auth) wi_argterm(sess->ws_auth);     /* etc */
   if((sess->ws_referer > sess->ws_rxbuf) && (sess->ws_referer < rxend))
      if (sess->ws_referer) wi_argterm(sess->ws_referer);
   if((sess->ws_uri > sess->ws_host) && (sess->ws_host < rxend))
      if (sess->ws_host) wi_argterm(sess->ws_host);
   


   for (i=0; i<38; i++)
   {
	   if (strcmp(strs[i],sess->ws_uri) == 0)
		   break;
   }

   if (i ==38)
   {
	  wi_senderr(sess, 333);
      return error;
   }


   /* Find and open file to return, */
   error = wi_fopen(sess, sess->ws_uri, "rb");


   if(error)
   {
      wi_senderr(sess, 404);  /* File not found */
      return error;
   }

   if((sess->ws_filelist == NULL) || 
      (sess->ws_filelist->wf_routines == NULL) ||
      (sess->ws_filelist->wf_fd == NULL))
   {
      dtrap();
      return WIE_BADFILE;
   }

   if(sess->ws_filelist->wf_routines->wfs_fauth)
   {
      int      admit;  /* 1 if OK, 0 if authentication fails */

      if(sess->ws_auth == NULL)  /* No auth info in http header */
      {
         admit = sess->ws_filelist->wf_routines->wfs_fauth(
            sess->ws_filelist->wf_fd, "", "");
      }
      else     /* Have auth info, parse it and check */
      {
         char name[32];
         char pass[32];

         wi_decode_auth(sess, name, sizeof(name), pass, sizeof(pass));

         admit = sess->ws_filelist->wf_routines->wfs_fauth(
            sess->ws_filelist->wf_fd, name, pass);
      }

      if(!admit)
      {
         wi_senderr(sess, 401);  /* Send "Auth required" reply  */
         wi_fclose(sess->ws_filelist);
         return WIE_PERMIT;
      }
   }


   /* Try to figure out if file may contain SSI or other content 
    * requiring server parsing. If not, mark it as binary. This 
    * will allow faster sending of images and other large binaries.
    */
   wi_setftype(sess);

   sess->ws_flags &= ~WF_HEADERSENT;   /* header not sent yet */

   if(cmd == H_GET)
   {
      /* start loading file to return. */
      sess->ws_state = WI_CONTENT;
      error = wi_readfile(sess);
      return error;
   }
   else  /* POST, wait for data */
   {
      sess->ws_state = WI_POSTRX;
      return 0;
   }
}

/* wi_badform()
 *
 * Called when a cgi function returns error text. send the text to the 
 * client and close the session.
 */

char * badformhead = "<html><head><title>Form Error</title> \
 <link href=\"praemio-style-main.css\" rel=\"stylesheet\" type=\"text/css\"></head> \
 <body onLoad=\"javascript:{ if(parent.frames[0]&&parent.frames['navig'].Go) parent.frames['navig'].Go()}\" > \
 <center><br><br><br><h2>";

char * badformtail = "</h2></body></html>";

void
wi_badform(wi_sess * sess, char * errmsg)
{
   wi_printf(sess, badformhead );
   wi_printf(sess, "Error in form: %s <br>", errmsg);
   wi_printf(sess, badformtail );

   wi_sockwrite(sess);

   return;
}


/* wi_readfile()
 *
 * Read file from disk or script into txbufs. Allocate txbufs as we go 
 * sess->ws_fd should have an open fd.
 *
 * BE VERY CAREFULL if you decide to edit this logic. The core loop is
 * rather convuluted since it's handling file reads & closes, 
 * chained txbufs, an exception for fast binary transfer, and calls to 
 * wi_ssi(). SSI are especially tricky since wi_ssi() will recursivly 
 * call back to wi_readfile()
 *
 * Returns: 0 if no error, else negative WIE_ error code.
 * 
 */

int
wi_readfile(struct wi_sess_s * sess)
{
   int         error;
   int         len;
   int         toread;
   wi_file *   fi;     /* info about current file */


   /* start loading file to return. */
   fi = sess->ws_filelist;

   /* Check for embedded form handlers */
   if(fi->wf_routines == &emfs)
   {
      EOFILE * eofile;
      em_file *   emf;

      eofile = (EOFILE *)fi->wf_fd;
      emf = eofile->eo_emfile;

      if(emf->em_flags & EMF_FORM)
      {
         char * (*formhandler)(wi_sess*);
         char * errmsg;

         formhandler = emf->em_routine;
         if(formhandler == NULL)
            return WIE_BADFILE;

         errmsg = formhandler(sess);
         if(errmsg)
         {
            wi_badform(sess, errmsg);
            return WIE_BADPARM;
         }
         if(sess->ws_filelist == NULL)    // done with request
            return 0;
         else
            fi = sess->ws_filelist;    // re-set local variable
      }
   }


readmore:
   toread = sizeof(fi->wf_data) - fi->wf_inbuf;
   len = wi_fread( &fi->wf_data[fi->wf_inbuf], 1, toread, fi );

   if(len <= 0)
   {
      wi_fclose(fi);

      /* See if there is another input file "outside" the current one.
       * This happens if the file we just closed was an SSI
       */
      if(sess->ws_filelist)
         return 0;
      else
         goto readdone;
   }

   sess->ws_last = cticks;
   fi->wf_inbuf += len;

   /* fast path for binary files. We've read first buffer from file
    * now - just jump to the sending code.
    */
   if(sess->ws_flags & WF_BINARY)
      goto readdone;

   /* Copy the file into a send buffer while searching for SSI strings */
   for(len = fi->wf_nextbuf; len < fi->wf_inbuf; len++)
   {
      if((fi->wf_data[len + 4] == '#') && 
         (fi->wf_data[len + 1] == '!'))
      {
         char * ssi_end;

         /* got complete SSI string? */
         ssi_end = strstr( &fi->wf_data[len], "-->");

         if(ssi_end)
         {
            int ssi_len = (ssi_end - &fi->wf_data[len]) + 3;

            fi->wf_nextbuf = len;      /* Set address of SSI text */

            if(strncmp( &fi->wf_data[len], "<!--#include", 12) == 0)
            {
               /* Call routine to process SSI string in file */
               error = wi_ssi(sess);
            }
            else if(strncmp( &fi->wf_data[len], "<!--#exec ", 10) == 0)
            {
               /* Call routine to process SSI string in file */
               error = wi_exec(sess);
            }

            /* Save location where SSI ends */
            len += ssi_len;

            /* break if SSI changed the current file. */
            if(sess->ws_filelist != fi)
            {
               fi->wf_nextbuf = len;
               return 0;
            }
            fi->wf_nextbuf = 0;  /* force jump to readmore */
         }
         else  /* end not found - SSI text may end in next block */
         {
            dtrap();
         }
      }

      /* Make sure we have space for char in txbuf */
      if((sess->ws_txbufs == NULL) ||
         (sess->ws_txtail->tb_total >= WI_TXBUFSIZE))
      {
         if(wi_txalloc(sess) == NULL)
            return WIE_MEMORY;
      }
      sess->ws_txtail->tb_data[sess->ws_txtail->tb_total++] = fi->wf_data[len];
   }

   /* See if we need to do more reading */
   if((fi->wf_nextbuf == 0) && (len > 0))
   {
      fi->wf_inbuf = 0;    /* no unread data in read buffer */
      goto readmore;
   }

readdone:

   /* Done with loading data, begin send process */
   sess->ws_state = WI_SENDDATA;
   error = wi_sockwrite(sess);

   return error;
}

/* wi_socketwrite()
 * 
 * This is called when a session has read all the data to send 
 * from files/scripts,and is ready to send it to socket.
 * 
 * Returns: 0 if no error, else negative WIE_ error code.
 */

int
wi_sockwrite(wi_sess * sess)
{
   txbuf *  txbuf;
   int      error;
   int      tosend;
   int      contentlen = 0;

   if(sess->ws_flags & WF_BINARY)
   {
      error = wi_movebinary(sess, sess->ws_filelist);
      return error;
   }

   if((sess->ws_flags & WF_HEADERSENT) == 0)   /* header sent yet? */
   {
      /* Build and prepend OK header - first calculate length. */
      for(txbuf = sess->ws_txbufs; txbuf; txbuf = txbuf->tb_next)
         contentlen += txbuf->tb_total;

      error = wi_replyhdr(sess, contentlen);
      if(error)
         return WIE_SOCKET;
   }

   while(sess->ws_txbufs) 
   {
      txbuf = sess->ws_txbufs;
      tosend = txbuf->tb_total - txbuf->tb_done;
      error = send(sess->ws_socket, &txbuf->tb_data[txbuf->tb_done], tosend, 0);
      if(error != tosend)
      {
         error = errno;
         if(error == EWOULDBLOCK)
         {
            txbuf->tb_done = 0;
            return 0;
         }
         dprintf("Socket write error %s\n", strerror(errno));
         dtrap(); 
         return WIE_SOCKET;
      }
      /* Fall to here if we sent the whole txbuf. Unlink & free it */
      sess->ws_txbufs = txbuf->tb_next;
      txbuf->tb_next = NULL;
      wi_txfree(txbuf);
      sess->ws_last = cticks;
   }

   /* fall to here when all txbufs are sent. */
   error = wi_txdone(sess);

   return error;
}

/* wi_redirect()
 * 
 * This is used by forms processing routines to return the passed file in
 * file in reply to a GET or POST request.
 *
 * Returns: 0 if no error, else negative WIE_ error code.
 * 
 */

int
wi_redirect(wi_sess * sess, char * filename)
{
   int      error;
   char *   pairs;

   sess->ws_referer = sess->ws_uri;
   sess->ws_uri = filename;
   sess->ws_last = cticks;

   /* unlink the completed form file fron the session */
   sess->ws_filelist = sess->ws_filelist->wf_next;

   /* parse any name/value pairs appended to file name */
   pairs = strchr(filename, '?');
   if(pairs)
   {
      *pairs++ = 0;     /* Null terminate URI field */
      error = wi_buildform(sess, pairs);
      if(error)
      {
         dtrap();       /* bad html from caller? */
         return error;
      }
   }

   /* Find and open new file to return, */
   error = wi_fopen(sess, filename, "rb");
   if(error)
   {
      wi_senderr(sess, 404);  /* File not found */
      return error;
   }

   sess->ws_state = WI_CONTENT;
   sess->ws_cmd = H_GET;
//   sess->ws_last = cticks;
   sess->ws_flags &= ~WF_HEADERSENT;

   return 0;   /* OK return */
}



/* wi_putfile()
 * 
 * This is called when a session receives a PUT command.
 * 
 *
 * Returns: 0 if no error, else negative WIE_ error code.
 * 
 */

int
wi_putfile( wi_sess * sess)
{

   dtrap();
   USE_ARG(sess);

   return 0;      /* No Error */
}

