// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


#
# Making the VNC applet.
#

CLASSES = VncViewer.class RfbProto.class AuthPanel.class VncCanvas.class \
	  OptionsFrame.class ClipboardFrame.class ButtonPanel.class \
	  DesCipher.class

PAGES = index.vnc shared.vnc noshared.vnc hextile.vnc zlib.vnc tight.vnc

all: $(CLASSES) VncViewer.jar

VncViewer.jar: $(CLASSES)
	@$(JavaArchive)

export:: $(CLASSES) VncViewer.jar $(PAGES)
	@$(ExportJavaClasses)

clean::
	$(RM) *.class *.jar
