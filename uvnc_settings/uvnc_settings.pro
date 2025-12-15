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
    uvnc_settings/capture.cpp \
    uvnc_settings/connections.cpp \
    uvnc_settings/createsfx2.cpp \
    uvnc_settings/createsfx3.cpp \
    uvnc_settings/dsmplugin.cpp \
    uvnc_settings/filetransfer.cpp \
    uvnc_settings/firewall.cpp \
    uvnc_settings/inifile.cpp \
    uvnc_settings/log.cpp \
    uvnc_settings/misc.cpp \
    uvnc_settings/network.cpp \
    uvnc_settings/security.cpp \
    uvnc_settings/service.cpp \
    uvnc_settings/stdafx.cpp \
    uvnc_settings/upnp.cpp \
    uvnc_settings/uvnc_settings.cpp \
    uvnc_settings/videodrivercheck.cpp \
    uvnc_settings/vncOSVersion.cpp \
    uvnc_settings/vncsetauth.cpp

HEADERS += \
    uvnc_settings/dsmplugin.h \
    uvnc_settings/firewall.h \
    uvnc_settings/inifile.h \
    uvnc_settings/log.h \
    uvnc_settings/resource.h \
    uvnc_settings/stdafx.h \
    uvnc_settings/upnp.h \
    uvnc_settings/uvnc_settings.h \
    uvnc_settings/vncOSVersion.h \
    uvnc_settings/vncsetauth.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    uvnc_settings/uvnc_settings/uvnc_settings.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

