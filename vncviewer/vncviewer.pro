QT = core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    AboutBox.cpp \
    AccelKeys.cpp \
    AuthDialog.cpp \
    buildtime.cpp \
    ClientConnection.cpp \
    ClientConnectionCacheRect.cpp \
    ClientConnectionClipboard.cpp \
    ClientConnectionCopyRect.cpp \
    ClientConnectionCoRRE.cpp \
    ClientConnectionCursor.cpp \
    ClientConnectionFile.cpp \
    ClientConnectionFullScreen.cpp \
    ClientConnectionHextile.cpp \
    ClientConnectionRaw.cpp \
    ClientConnectionRRE.cpp \
    ClientConnectionRSAAES.cpp \
    ClientConnectionTight.cpp \
    ClientConnectionTLS.cpp \
    ClientConnectionUltra.cpp \
    ClientConnectionUltra2.cpp \
    ClientConnectionZlib.cpp \
    ClientConnectionZlibHex.cpp \
    Daemon.cpp \
    display.cpp \
    Exception.cpp \
    FileTransfer.cpp \
    FullScreenTitleBar.cpp \
    InfoBox.cpp \
    KeyMap.cpp \
    KeyMapjap.cpp \
    Log.cpp \
    LowLevelHook.cpp \
    main.cpp \
    MessBox.cpp \
    MRU.cpp \
    SessionDialog.cpp \
    sessiondialogLoadSave.cpp \
    SessionDialogTabs.cpp \
    Snapshot.cpp \
    stdhdrs.cpp \
    TextChat.cpp \
    UltraVNCHerperFunctions.cpp \
    vncauth.c \
    VNCOptions.cpp \
    vnctouch.cpp \
    vncviewer.cpp \
    VNCviewerApp.cpp \
    VNCviewerApp32.cpp \
    vncviewerqt.cpp \
    xz.cpp \
    zrle.cpp \
    directx/directxviewer.cpp

HEADERS += \
    AboutBox.h \
    AccelKeys.h \
    AuthDialog.h \
    ClientConnection.h \
    Daemon.h \
    display.h \
    Exception.h \
    FileTransfer.h \
    FpsCounter.h \
    FullScreenTitleBar.h \
    FullScreenTitleBarConst.h \
    KeyMap.h \
    KeyMapjap.h \
    keysym.h \
    keysymdef.h \
    keysymdefjap.h \
    Log.h \
    LowLevelHook.h \
    MEssBox.h \
    MRU.h \
    multimon.h \
    rfb.h \
    SessionDialog.h \
    Snapshot.h \
    stdhdrs.h \
    TextChat.h \
    UltraVNCHelperFunctions.h \
    vncauth.h \
    VNCOptions.h \
    vnctouch.h \
    vncviewer.h \
    VNCviewerApp.h \
    VNCviewerApp32.h \
    vncviewerqt.h \
    directx/directxviewer.h \
    res/resource.h

FORMS += \
    vncviewerqt.ui

TRANSLATIONS += \
    vncviewerQt_fr_FR.ts

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    res/build-bcc32.bat \
    res/vncviewer.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

