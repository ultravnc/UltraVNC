QT = core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    setpasswd/inifile.cpp \
    setpasswd/setpasswd.cpp \
    setpasswd/stdafx.cpp

HEADERS += \
    setpasswd/inifile.h \
    setpasswd/resource.h \
    setpasswd/stdafx.h \
    setpasswd/targetver.h

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

