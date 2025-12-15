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
    ms-logon/authadm/authadmin.cpp \
    ms-logon/authSSP/authSSP.cpp \
    ms-logon/authSSP/EventLogging.cpp \
    ms-logon/authSSP/GenClientServerContext.cpp \
    ms-logon/authSSP/vncAccessControl.cpp \
    ms-logon/authSSP/vncSecurityEditor.cpp \
    ms-logon/authSSP/vncSSP.cpp \
    ms-logon/ldapauth/ldapAuth.cpp \
    ms-logon/ldapauth9x/ldapAuth9x.cpp \
    ms-logon/ldapauthNT4/ldapAuthnt4.cpp \
    ms-logon/logging/logging.cpp \
    ms-logon/MSLogonACL/MSLogonACL.cpp \
    ms-logon/MSLogonACL/vncExportACL.cpp \
    ms-logon/MSLogonACL/vncImportACL.cpp \
    ms-logon/testauth/ntlogon.cpp \
    ms-logon/workgrpdomnt4/workgrpdomnt4.cpp

HEADERS += \
    versioninfo.h \
    eventMessageLogger/messages.h \
    ms-logon/authadm/authadmin.h \
    ms-logon/authadm/resource.h \
    ms-logon/authSSP/Auth_Seq.h \
    ms-logon/authSSP/authSSP.h \
    ms-logon/authSSP/buildtime.h \
    ms-logon/authSSP/EventLogging.h \
    ms-logon/authSSP/GenClientServerContext.h \
    ms-logon/authSSP/resource.h \
    ms-logon/authSSP/vncAccessControl.h \
    ms-logon/authSSP/vncSecurityEditor.h \
    ms-logon/authSSP/vncSecurityEditorProps.h \
    ms-logon/authSSP/vncSSP.h \
    ms-logon/ldapauth/ldapAuth.h \
    ms-logon/ldapauth/resource.h \
    ms-logon/ldapauth9x/ldapAuth9x.h \
    ms-logon/ldapauth9x/resource.h \
    ms-logon/ldapauthNT4/ldapAuthnt4.h \
    ms-logon/ldapauthNT4/resource.h \
    ms-logon/logging/logging.h \
    ms-logon/logging/resource.h \
    ms-logon/MSLogonACL/buildtime.h \
    ms-logon/MSLogonACL/MSLogonACL.h \
    ms-logon/MSLogonACL/resource.h \
    ms-logon/MSLogonACL/vncExportACL.h \
    ms-logon/MSLogonACL/vncImportACL.h \
    ms-logon/testauth/resource.h \
    ms-logon/workgrpdomnt4/resource.h \
    ms-logon/workgrpdomnt4/workgrpdomnt4.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    eventMessageLogger/messages.rc \
    ms-logon/authadm/authadmin.qrc \
    ms-logon/authadm/authadmin.rc \
    ms-logon/authSSP/authSSP.rc \
    ms-logon/ldapauth/ldapAuth.rc \
    ms-logon/ldapauth9x/ldapAuth9x.rc \
    ms-logon/ldapauthNT4/ldapAuthnt4.rc \
    ms-logon/logging/logging.rc \
    ms-logon/MSLogonACL/MSLogonACL.rc \
    ms-logon/testauth/testauth.rc \
    ms-logon/workgrpdomnt4/workgrpdomnt4.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

