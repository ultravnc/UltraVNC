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


#
# Making the VNC applet.
#

CP = cp
JC = javac
JAR = jar
ARCHIVE = VncViewer.jar
PAGES = index.vnc shared.vnc noshared.vnc hextile.vnc zlib.vnc tight.vnc
INSTALL_DIR = /usr/local/vnc/classes

CLASSES = VncViewer.class RfbProto.class AuthPanel.class VncCanvas.class \
	  OptionsFrame.class ClipboardFrame.class ButtonPanel.class \
	  DesCipher.class RecordingFrame.class SessionRecorder.class DH.class

SOURCES = VncViewer.java RfbProto.java AuthPanel.java VncCanvas.java \
	  OptionsFrame.java ClipboardFrame.java ButtonPanel.java \
	  DesCipher.java RecordingFrame.java SessionRecorder.java DH.java

all: $(CLASSES) $(ARCHIVE)

$(CLASSES): $(SOURCES)
	$(JC) -O $(SOURCES)

$(ARCHIVE): $(CLASSES)
	$(JAR) cf $(ARCHIVE) $(CLASSES)

install: $(CLASSES) $(ARCHIVE)
	$(CP) $(CLASSES) $(ARCHIVE) $(PAGES) $(INSTALL_DIR)

export:: $(CLASSES) $(ARCHIVE) $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar
