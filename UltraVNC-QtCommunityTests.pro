
QT = core gui widgets #Fix Error 'QMainWindow' file not found and "Project ERROR: Unknown module(s) in QT: Core" don't use "Core" use "core" respect case

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    common/Clipboard.cpp \
    common/d3des.c \
    common/Hyperlinks.cpp \
    common/mn_wordlist.c \
    common/mnemonic.c \
    common/UltraVncZ.cpp \
    common/win32_helpers.cpp \
    DSMPlugin/DSMPlugin.cpp \
    DSMPlugin/MSRC4Plugin/crypto.cpp \
    DSMPlugin/MSRC4Plugin/EnvReg.cpp \
    DSMPlugin/MSRC4Plugin/logging.cpp \
    DSMPlugin/MSRC4Plugin/main.cpp \
    DSMPlugin/MSRC4Plugin/MSRC4Plugin.cpp \
    DSMPlugin/MSRC4Plugin/myDebug.cpp \
    DSMPlugin/MSRC4Plugin/registry.cpp \
    DSMPlugin/MSRC4Plugin/StdAfx.cpp \
    DSMPlugin/MSRC4Plugin/utils.cpp \
    DSMPlugin/TestPlugin/StdAfx.cpp \
    DSMPlugin/TestPlugin/TestPlugin.cpp \
    lzo/minilzo.c \
    rfb/dh.cpp \
    rfb/vncauth.c \
    rfb/xzywtemplate.c \
    rfb/zywrletemplate.c \
    ZipUnZip32/ZipUnzip32.cpp

HEADERS += \
    common/Clipboard.h \
    common/d3des.h \
    common/Hyperlinks.h \
    common/mnemonic.h \
    common/rfb.h \
    common/ScopeGuard.h \
    common/stdhdrs.h \
    common/UltraVncZ.h \
    common/VersionHelpers.h \
    common/win32_helpers.h \
    DSMPlugin/DSMPlugin.h \
    DSMPlugin/MSRC4Plugin/crypto.h \
    DSMPlugin/MSRC4Plugin/EnvReg.h \
    DSMPlugin/MSRC4Plugin/logging.h \
    DSMPlugin/MSRC4Plugin/main.h \
    DSMPlugin/MSRC4Plugin/MSRC4Plugin.h \
    DSMPlugin/MSRC4Plugin/myDebug.h \
    DSMPlugin/MSRC4Plugin/registry.h \
    DSMPlugin/MSRC4Plugin/resource.h \
    DSMPlugin/MSRC4Plugin/StdAfx.h \
    DSMPlugin/MSRC4Plugin/utils.h \
    DSMPlugin/MSRC4Plugin/version.h \
    DSMPlugin/TestPlugin/resource.h \
    DSMPlugin/TestPlugin/StdAfx.h \
    DSMPlugin/TestPlugin/TestPlugin.h \
    lzo/lzoconf.h \
    lzo/lzodefs.h \
    lzo/minilzo.h \
    rfb/dh.h \
    rfb/gii.h \
    rfb/rfbproto.h \
    rfb/vncauth.h \
    rfb/xzDecode.h \
    rfb/xzEncode.h \
    rfb/zrleDecode.h \
    rfb/zrleEncode.h \
    ZipUnZip32/ZipUnZip32.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    DSMPlugin/MSRC4Plugin/crypto.rc \
    DSMPlugin/MSRC4Plugin/MSRC4Plugin.rc \
    DSMPlugin/MSRC4Plugin/version.rc2 \
    DSMPlugin/TestPlugin/TestPlugin.rc \
    JavaViewer/mk.bat \
    JavaViewer/run.bat \
    JavaViewer/runapplet.bat \

SUBDIRS += \
    addon \
    avilog \
    omnithread \
    rdr \
    repeater \
    setcad \
    setpasswd \
    udt4\win \
    UdtCloudlib \
    uvnc_settings \
    uvnckeyboardhelper \
    vncviewer \
    winvnc \
    zipunzip_src\unzip\windll\vc6

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

