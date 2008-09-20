
     Ultr@VNC 1.0.5 release

     Copyright (C) 2002-2008 Ultr@VNC Team - All rights reserved

**********************************************************************

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
  USA.

  If the source code for the program is not available from the place from
  which you received this file, check
  http://www.uvnc.com/

**********************************************************************

 The authors shall not in any way be liable for any damage or legal
 consequences as a result of using this software. We make absolutly no
 warranties about the reliability of this software. Use it at your own
 risks !

**********************************************************************



****** Table of content **********************************************

 1. Introduction
 2. Features
 3. List of included files
 4. Versions History
 5. KNOWN ISSUES
 6. TIPS
 7. Details




1. *** Introduction **************************************************

 VNC is a great and famous remote controle, multi-OS tool, created at
 the ATT Research Labs de Cambridge-http://www.uk.research.att.com/vnc

 This Ultr@VNC version is based on:
 - RealVNC 336 & 337
   and includes:
 - Most of eSVNC 1.1.2 r2 functionnalities
 - Most of Vdacc-VNC functionnalities
 - Cursor handling code and Tight encoding from TightVNC
 - VNC QuickDesk toolbar code
 - And various code modifications coming from talented developpers.

 - Since v1.0.0 RC 12, Ultr@VNC FileTransfer can transfer whole Directories.
   For this it uses "zip32.dll" and "unzip32.dll" dll files and source code
   coming from the Info-Zip group. Please see at the end of this
   document (section 7.) for details about these dlls.

 - Since v1.0.0 RC 12, the viewer Toolbar looks professionnal, thanks to
   Lim Chee Aun (http://www.phoenity.com/) who has designed it.

 Ultr@VNC brings tons of functionnalities and high performances even
 over modem connections.

 ------------------------------------------------------------------------
 Many thanks to all people who help us developping, testing, stabilizing,
 answering users questions and animating the forum and the mailing list
 ------------------------------------------------------------------------



2. *** Features *****************************************************

* Supported Operating Systems: 2000/XP/2003/VISTA
       For Win9x/Me/NT4 please use version <= 1.0.2

* Auto configuration and Quick Options for easy connections.

* Viewer Toolbar for a quick access to the main functions and greatly
improved ergonomy.

* High Speed. On W2000, XP and Vista, Ultr@VNC can use an additional and optional
Video Hook Driver (aka Mirror Driver) that dramatically improves performances
and reduces CPU activity over LAN connections. So when the bandwidth
is good - typically over a LAN - Ultr@VNC lets you work on a remote
computer with an incredible 'real-time' feeling (hundreds of screen
updates per second), just as if you were sitting in front of it. A
WinVNC server under Win9x uses the standard hookdll in combination
with ddi hooking to improve the updates
handling. Ultr@VNC also features an ultra fast Fullscreen Polling
mode without any additionnal driver.
Whatever the connection speed, Ultr@VNC brings you optimal comfort.
Note that this driver is distributed separatly from Ultr@VNC.

* Embedded File Transfer with intuitive Graphical User Interface allowing
for easy file copy between local and remote computers. It uses the current
VNC connection and files are compressed during their transfer.
File transfers can be asynchronous so screen updates can continue while 
a file is being transfered in background.
The delta transfer mode allows to resume interrupted transfers.

* MS Logon/NT security support. You can manage server access using MS Users,
Domains and Groups. It also includes a logging feature where all actions are
written to a log file.

* Bandwidth Saving Strategies that provide optimal responsiveness over slow
connections: Server Screen Scaling, Cache Management , Local Cursor handling,
reduced colors modes (256, 64, 8, Grey scales).

* Advanced JavaViewer featuring FileTransfer, MSLogon and reduced color modes.

* Data Stream Modification Plugin System allowing any kind of operation on the
data exchanged between client and server, from an external DLL (independant,
not linked and not distributed with Ultr@VNC): additionnal rights checking,
communication timing, data recording/persitence, encryption...
it's up to the DLL developper.

* Optional "always-on-top" Viewer Status Window dynamically displaying
connection informations.

* Various View Modes including Full-Screen, Scaled and Windowed. Full-Screen
mode lets you see the remote screen on the entire screen of your display.
Scaled Viewer mode lets you see the scaled remote screen in a window with a
user defined size. Scaled Server mode generates less network traffic from
server side and use a pixel blending algorithm to optimize the display.
Fuzzy Mode combines Server and Viewer scaling to provide reasonable visual
comfort and speed even over very slow connections.
The Autoscaling mode scales the view so it fits the viewer screen.

* Dynamic Single Window/Full Desktop view switching, from viewer side.
* Server's Desktop resolution switching without disconnection.
* Dynamic Mouse and Keyboard locking on remote server, from viewer side.
* Server Screen Blanking, from viewer side

* Embedded Client/Server Text Chat.

* Possibility to send Ctrl-Alt-Del to the remote server
(when WinVNC is run as a service), or any other special key.

* Support for 32/24/16/8 bits colors.
Both Ultr@VNC Viewer and Server are compatible with RealVNC, TightVNC, eSVNC,
PalmVNC2 (with server scaling feature)...
Ultr@VNC server can work as a Service under all supported operating systems,
allowing remote user Logon/Logoff as well as Remote Shutdown.

* Connectivity: Standard Viewer and HTTP JavaViewer connections over TCP/IP,
as well as XDMCP direct X11 connection (still experimental).





3. *** List of included files ****************************************

 - readme.txt           : This text file
 - License.txt          : GNU General Public License. PLEASE Read it !
 - Whatsnew.txt         : Latest modifs, fixes, features...

 - WinVNC.exe           : Ultr@VNC Server
 - VNCHooks.dll         : Library used in WinVNC.exe

 - vncviewer.exe        : Ultr@VNC Viewer

 - Vncddihk.dll         : Win9x video hook driver dll
 - 16bithlp.exe         : Win9x video hook driver exe

 - repeater.exe         : The repeater exe

 - VNCHooks_settings.reg : Default Registry Settings for WinVNC

 * These 7 files are used with WinVNC for MS Logon functionnalities
 over the various Win versions and configurations:

 - logging.dll          : needed, does event and file logging
 - authadmin.dll        : if present, give localadmin access
 - workgrpdomnt4.dll    : Workgroup and NT4 domain checking
 - ldapauth.dll         : Active directory W2K up
 - ldapauth9x.dll       : Active Directory 9X
 - ldapauthnt4.dll      : Active Directory NT4
 - authSSP              : MS-Logon II (aka New MS-Logon)
 - MSLogonACL.exe       : Import/export utility for MS-Logon II ACL
 - Logmessages.dll      : log messages handling

 - zip32.dll            : These 2 dlls are used by vncviewer.exe and
 - unzip32.dll          : WinVNC to perform Directory Transfers

 * Depending on he language you've chosen, you can get one off several
 files:

 - lang.dll
 - french.dll
 - german.dll
 - ...

 * Depending on the package, the following files may be included as well,
   possibly in subdirectories:

 - Others Languages: French & German vncviewer.exe, French WinVNC.exe ...
 
 - DSM Encryption Plugin : MSRC4 plugin, also available here with 
                           documentation:
                           http://msrc4plugin.home.comcast.net/index.html

			   The latest version is included in this package,
                           in the 'plugins' subdiretory. Please see the 
                           correspondin readme.txt, whatsnew.txt for details

			   The .dsm file is also copied in the
                           ultravnc main install directory so it can be 
                           used directly.
                           
 - Additional optional files (GPL or non-GPL) can be downloaded and 
   installed during the setup process, such as the video mirror driver 
   and the addons for Vista. These files are not mandatory to make UltraVNC 
   work but they generally improve the performances and user experience. 
   These additional files can also be downloaded separately here:
      http://www.uvnc.com/download/		



 4. Versions History

   Please see the whatsnew.txt file for versions history



 5. KNOWN ISSUES

    Please see Ultr@VNC Forum and Mailing list



 6. TIPS

   Please see Ultr@VNC Web site and Forum



 7. Details

 * How does Ultr@VNC use Info-Zip

   Ultra WinVNC and vncviewer both use 2 dlls for Directory Transfer:

   * Zip32.dll
   It is the original Info-Zip Zip 2.3 version (without encryption) that
   can be found on the Info-Zip sites, as well as its source code:
   http://www.info-zip.org/pub/infozip/
   http://www.cdrom.com/pub/infozip/
   ftp://ftp.info-zip.org/pub/infozip

   * Unzip32.dll
   Has been compiled using the portable unzip550 source code that can be
   found on the Info-Zip sites:
   http://www.info-zip.org/pub/infozip/
   http://www.cdrom.com/pub/infozip/
   ftp://ftp.info-zip.org/pub/infozip
   THE ONLY thing that was modified is that the "crypt.c" and "crypt.h"
   files have been replaced with the dummy (null) ones coming from the
   Zip 2.3 sources. So this unzip32.dll IS NOT the original Info-Zip
   "unzip32.dll" and does not contain any encryption/decryption code or binary.

   ---------------------------------------------------------------------------
   For any bug/pb regarding the use of these dlls by Ultr@VNC (Directory
   Transfer), please only send mails to Ultr@VNC team (http://ultravnc.sf.net)
   ---------------------------------------------------------------------------


 * Following, the Info-Zip full License

This is version 2003-May-08 of the Info-ZIP copyright and license.
The definitive version of this document should be available at
ftp://ftp.info-zip.org/pub/infozip/license.html indefinitely.


Copyright (c) 1990-2003 Info-ZIP.  All rights reserved.

For the purposes of this copyright and license, "Info-ZIP" is defined as
the following set of individuals:

   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
   Jean-loup Gailly, Hunter Goatley, Ian Gorman, Chris Herborth, Dirk Haase,
   Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz, David Kirschbaum,
   Johnny Lee, Onno van der Linden, Igor Mandrichenko, Steve P. Miller,
   Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs, Kai Uwe Rommel,
   Steve Salisbury, Dave Smith, Christian Spieler, Antoine Verheijen,
   Paul von Behren, Rich Wales, Mike White

This software is provided "as is," without warranty of any kind, express
or implied.  In no event shall Info-ZIP or its contributors be held liable
for any direct, indirect, incidental, special or consequential damages
arising out of the use of or inability to use this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. Redistributions of source code must retain the above copyright notice,
       definition, disclaimer, and this list of conditions.

    2. Redistributions in binary form (compiled executables) must reproduce
       the above copyright notice, definition, disclaimer, and this list of
       conditions in documentation and/or other materials provided with the
       distribution.  The sole exception to this condition is redistribution
       of a standard UnZipSFX binary (including SFXWiz) as part of a
       self-extracting archive; that is permitted without inclusion of this
       license, as long as the normal SFX banner has not been removed from
       the binary or disabled.

    3. Altered versions--including, but not limited to, ports to new operating
       systems, existing ports with new graphical interfaces, and dynamic,
       shared, or static library versions--must be plainly marked as such
       and must not be misrepresented as being the original source.  Such
       altered versions also must not be misrepresented as being Info-ZIP
       releases--including, but not limited to, labeling of the altered
       versions with the names "Info-ZIP" (or any variation thereof, including,
       but not limited to, different capitalizations), "Pocket UnZip," "WiZ"
       or "MacZip" without the explicit permission of Info-ZIP.  Such altered
       versions are further prohibited from misrepresentative use of the
       Zip-Bugs or Info-ZIP e-mail addresses or of the Info-ZIP URL(s).

    4. Info-ZIP retains the right to use the names "Info-ZIP," "Zip," "UnZip,"
       "UnZipSFX," "WiZ," "Pocket UnZip," "Pocket Zip," and "MacZip" for its
       own source and binary releases.


