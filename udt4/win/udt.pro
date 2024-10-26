QT = core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../app/appclient.cpp \
    ../app/appserver.cpp \
    ../app/recvfile.cpp \
    ../app/sendfile.cpp \
    ../app/test.cpp \
    ../src/api.cpp \
    ../src/buffer.cpp \
    ../src/cache.cpp \
    ../src/ccc.cpp \
    ../src/channel.cpp \
    ../src/common.cpp \
    ../src/core.cpp \
    ../src/epoll.cpp \
    ../src/list.cpp \
    ../src/md5.cpp \
    ../src/packet.cpp \
    ../src/queue.cpp \
    ../src/window.cpp

HEADERS += \
    ../app/cc.h \
    ../app/test_util.h \
    ../src/api.h \
    ../src/buffer.h \
    ../src/cache.h \
    ../src/ccc.h \
    ../src/channel.h \
    ../src/common.h \
    ../src/core.h \
    ../src/epoll.h \
    ../src/list.h \
    ../src/md5.h \
    ../src/packet.h \
    ../src/queue.h \
    ../src/udt.h \
    ../src/window.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

