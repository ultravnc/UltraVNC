QT = core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    FdInStream.cxx \
    FdOutStream.cxx \
    InStream.cxx \
    NullOutStream.cxx \
    xzInStream.cxx \
    xzOutStream.cxx \
    ZlibInStream.cxx \
    ZlibOutStream.cxx \
    ZstdInStream.cxx \
    ZstdOutStream.cxx

HEADERS += \
    Exception.h \
    FdInStream.h \
    FdOutStream.h \
    FixedMemOutStream.h \
    InStream.h \
    MemInStream.h \
    MemOutStream.h \
    NullOutStream.h \
    OutStream.h \
    types.h \
    xzInStream.h \
    xzOutStream.h \
    ZlibInStream.h \
    ZlibOutStream.h \
    ZstdInStream.h \
    ZstdOutStream.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

