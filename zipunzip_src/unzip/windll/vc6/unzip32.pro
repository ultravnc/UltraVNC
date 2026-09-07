QT = core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../api.c \
    ../../apihelp.c \
    ../../crc32.c \
    ../../crctab.c \
    ../../crypt.c \
    ../../envargs.c \
    ../../explode.c \
    ../../extract.c \
    ../../fileio.c \
    ../../funzip.c \
    ../../gbloffs.c \
    ../../globals.c \
    ../../inflate.c \
    ../../list.c \
    ../../match.c \
    ../../process.c \
    ../../timezone.c \
    ../../ttyio.c \
    ../../unreduce.c \
    ../../unshrink.c \
    ../../unzip.c \
    ../../unzipstb.c \
    ../../zipinfo.c \
    ../../win32/crc_i386.c \
    ../../win32/nt.c \
    ../../win32/win32.c \
    ../../windll/uzexampl.c \
    ../../windll/windll.c \
    ../../../zip20/api.c \
    ../../../zip20/crc32.c \
    ../../../zip20/crctab.c \
    ../../../zip20/crypt.c \
    ../../../zip20/deflate.c \
    ../../../zip20/fileio.c \
    ../../../zip20/globals.c \
    ../../../zip20/timezone.c \
    ../../../zip20/trees.c \
    ../../../zip20/ttyio.c \
    ../../../zip20/util.c \
    ../../../zip20/zip.c \
    ../../../zip20/zipcloak.c \
    ../../../zip20/zipfile.c \
    ../../../zip20/zipnote.c \
    ../../../zip20/zipsplit.c \
    ../../../zip20/zipup.c \
    ../../../zip20/win32/crc_i386.c \
    ../../../zip20/win32/nt.c \
    ../../../zip20/win32/win32.c \
    ../../../zip20/win32/win32zip.c \
    ../../../zip20/windll/example.c \
    ../../../zip20/windll/windll.c \

HEADERS += \
    ../../consts.h \
    ../../crypt.h \
    ../../ebcdic.h \
    ../../globals.h \
    ../../inflate.h \
    ../../tables.h \
    ../../timezone.h \
    ../../ttyio.h \
    ../../unzip.h \
    ../../unzpriv.h \
    ../../unzvers.h \
    ../../zip.h \
    ../../win32/nt.h \
    ../../win32/rsxntwin.h \
    ../../win32/w32cfg.h \
    ../decs.h \
    ../structs.h \
    ../uzexampl.h \
    ../windll.h \
    ../../../zip20/api.h \
    ../../../zip20/crypt.h \
    ../../../zip20/ebcdic.h \
    ../../../zip20/revision.h \
    ../../../zip20/tailor.h \
    ../../../zip20/timezone.h \
    ../../../zip20/ttyio.h \
    ../../../zip20/zip.h \
    ../../../zip20/ziperr.h \
    ../../../zip20/win32/nt.h \
    ../../../zip20/win32/osdep.h \
    ../../../zip20/win32/rsxntwin.h \
    ../../../zip20/win32/win32zip.h \
    ../../../zip20/win32/zipup.h \
    ../../../zip20/windll/example.h \
    ../../../zip20/windll/resource.h \
    ../../../zip20/windll/structs.h \
    ../../../zip20/windll/windll.h \
    ../../../zip20/windll/zipver.h \

FORMS += \

TRANSLATIONS += \

CONFIG += lrelease
CONFIG += embed_translations

RESOURCES += \
    ../windll.rc \
    ../../../zip20/windll/windll.rc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

