# // This file is part of UltraVNC
# // https://github.com/ultravnc/UltraVNC
# // https://uvnc.com/
# //
# // SPDX-License-Identifier: GPL-3.0-or-later
# //
# // SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
# // SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
# //


QT = core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    gui.cpp \
    lists_functions.cpp \
    mode12_listener.cpp \
    mode2_listener_server.cpp \
    repeater.cpp \
    socket_functions.cpp \
    webgui/settings.c \
    webgui/webclib.c \
    webgui/webfs.c \
    webgui/webio.c \
    webgui/webobjs.c \
    webgui/websys.c \
    webgui/webtest.c \
    webgui/webutils.c \
    webgui/wsfcode.c \
    webgui/wsfdata.c

HEADERS += \
    list_functions.h \
    repeater.h \
    resource.h \
    resources.h \
    webgui/linuxdefs.h \
    webgui/webfs.h \
    webgui/webgui.h \
    webgui/webio.h \
    webgui/websys.h \
    webgui/windowsdefs.h \
    webgui/wsfdata.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    resources.rc \
    webgui/buildfs.bat

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

