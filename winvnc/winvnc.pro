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
    createpassword/createpassword.cpp \
    loadmemory/loadDllFromMemory.cpp \
    loadmemory/MemoryModule.c \
    vnchooks/SharedData.cpp \
    vnchooks/VNCHooks.cpp \
    winvnc/benchmark.cpp \
    winvnc/buildtime.cpp \
    winvnc/cadthread.cpp \
    winvnc/CloudDialog.cpp \
    winvnc/CpuUsage.cpp \
    winvnc/credentials.cpp \
    winvnc/DeskdupEngine.cpp \
    winvnc/Dtwinver.cpp \
    winvnc/getinfo.cpp \
    winvnc/HelperFunctions.cpp \
    winvnc/HideDesktop.cpp \
    winvnc/inifile.cpp \
    winvnc/initipp.cpp \
    winvnc/IPC.cpp \
    winvnc/LayeredWindows.cpp \
    winvnc/MouseSimulator.cpp \
    winvnc/PropertiesDialog.cpp \
    winvnc/rfbRegion_win32.cpp \
    winvnc/rfbRegion_X11.cxx \
    winvnc/rfbUpdateTracker.cpp \
    winvnc/RulesListView.cpp \
    winvnc/ScreenCapture.cpp \
    winvnc/ScSelect.cpp \
    winvnc/service.cpp \
    winvnc/SettingsManager.cpp \
    winvnc/stdhdrs.cpp \
    winvnc/tableinitcmtemplate.cpp \
    winvnc/tableinittctemplate.cpp \
    winvnc/tabletranstemplate.cpp \
    winvnc/TextChat.cpp \
    winvnc/Timer.cpp \
    winvnc/translate.cpp \
    winvnc/UdpEchoServer.cpp \
    winvnc/UltraVNCService.cpp \
    winvnc/uvncUiAccess.cpp \
    winvnc/videodriver.cpp \
    winvnc/videodrivercheck.cpp \
    winvnc/VirtualDisplay.cpp \
    winvnc/vistahook.cpp \
    winvnc/vncabout.cpp \
    winvnc/vncacceptdialog.cpp \
    winvnc/vncauth.c \
    winvnc/vncbuffer.cpp \
    winvnc/vncclient.cpp \
    winvnc/vncconndialog.cpp \
    winvnc/vncdesktop.cpp \
    winvnc/vncdesktopsink.cpp \
    winvnc/vncDesktopSW.cpp \
    winvnc/vncdesktopthread.cpp \
    winvnc/vncencodecorre.cpp \
    winvnc/vncencodehext.cpp \
    winvnc/vncencoder.cpp \
    winvnc/vncencoderCursor.cpp \
    winvnc/vncencoderre.cpp \
    winvnc/vncEncodeTight.cpp \
    winvnc/vncEncodeUltra.cpp \
    winvnc/vncEncodeUltra2.cpp \
    winvnc/vncEncodeXZ.cpp \
    winvnc/vncEncodeZlib.cpp \
    winvnc/vncEncodeZlibHex.cpp \
    winvnc/vncencodezrle.cpp \
    winvnc/vnchttpconnect.cpp \
    winvnc/vncinsthandler.cpp \
    winvnc/vnckeymap.cpp \
    winvnc/vncListDlg.cpp \
    winvnc/vnclog.cpp \
    winvnc/vnclogon.cpp \
    winvnc/vncmenu.cpp \
    winvnc/vncMultiMonitor.cpp \
    winvnc/vncntlm.cpp \
    winvnc/vncOSVersion.cpp \
    winvnc/vncserver.cpp \
    winvnc/vncsetauth.cpp \
    winvnc/vncsockconnect.cpp \
    winvnc/vnctimedmsgbox.cpp \
    winvnc/vsocket.cpp \
    winvnc/winvnc.cpp

HEADERS += \
    loadmemory/loadDllFromMemory.h \
    loadmemory/MemoryModule.h \
    vnchooks/resource.h \
    vnchooks/SharedData.h \
    vnchooks/VNCHooks.h \
    winvnc/cadthread.h \
    winvnc/CloudDialog.h \
    winvnc/CpuUsage.h \
    winvnc/credentials.h \
    winvnc/DeskdupEngine.h \
    winvnc/Dtwinver.h \
    winvnc/HelperFunctions.h \
    winvnc/HideDesktop.h \
    winvnc/inifile.h \
    winvnc/IPC.h \
    winvnc/keysymdef.h \
    winvnc/LayeredWindows.h \
    winvnc/Localization.h \
    winvnc/minmax.h \
    winvnc/MouseSimulator.h \
    winvnc/PropertiesDialog.h \
    winvnc/resource.h \
    winvnc/rfb.h \
    winvnc/rfbMisc.h \
    winvnc/rfbRect.h \
    winvnc/rfbRegion.h \
    winvnc/rfbRegion_win32.h \
    winvnc/rfbRegion_X11.h \
    winvnc/rfbUpdateTracker.h \
    winvnc/RulesListView.h \
    winvnc/ScreenCapture.h \
    winvnc/ScSelect.h \
    winvnc/SettingsManager.h \
    winvnc/stdhdrs.h \
    winvnc/TextChat.h \
    winvnc/Timer.h \
    winvnc/translate.h \
    winvnc/UdpEchoServer.h \
    winvnc/UltraVNCService.h \
    winvnc/uvncUiAccess.h \
    winvnc/videodriver.h \
    winvnc/VirtualDisplay.h \
    winvnc/vncabout.h \
    winvnc/vncacceptdialog.h \
    winvnc/vncauth.h \
    winvnc/vncbuffer.h \
    winvnc/vncclient.h \
    winvnc/vncconndialog.h \
    winvnc/vncdesktop.h \
    winvnc/vncdesktopthread.h \
    winvnc/vncencodecorre.h \
    winvnc/vncencodehext.h \
    winvnc/vncencodemgr.h \
    winvnc/vncencoder.h \
    winvnc/vncencoderre.h \
    winvnc/vncEncodeTight.h \
    winvnc/vncEncodeUltra.h \
    winvnc/vncEncodeUltra2.h \
    winvnc/vncEncodeXZ.h \
    winvnc/vncEncodeZlib.h \
    winvnc/vncEncodeZlibHex.h \
    winvnc/vncencodezrle.h \
    winvnc/vnchttpconnect.h \
    winvnc/vncinsthandler.h \
    winvnc/vnckeymap.h \
    winvnc/vncListDlg.h \
    winvnc/vnclog.h \
    winvnc/vnclogon.h \
    winvnc/vncmemcpy.h \
    winvnc/vncmenu.h \
    winvnc/vncOSVersion.h \
    winvnc/vncpasswd.h \
    winvnc/vncserver.h \
    winvnc/vncsetauth.h \
    winvnc/vncsockconnect.h \
    winvnc/vnctimedmsgbox.h \
    winvnc/vsocket.h \
    winvnc/vtypes.h \
    winvnc/winvnc.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    vnchooks/vnchooks.rc \
    winvnc/winvnc.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

