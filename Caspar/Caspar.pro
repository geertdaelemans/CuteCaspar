#-------------------------------------------------
#
# Project created by QtCreator 2019-07-31T11:26:41
#
#-------------------------------------------------

QT       -= gui
QT       += network

TARGET = Caspar
TEMPLATE = lib

DEFINES += CASPAR_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        AmcpDevice.cpp \
        CasparDevice.cpp \
        Models/CasparData.cpp \
        Models/CasparMedia.cpp \
        Models/CasparTemplate.cpp \
        Models/CasparThumbnail.cpp

HEADERS += \
        AmcpDevice.h \
        CasparDevice.h \
        Models/CasparData.h \
        Models/CasparMedia.h \
        Models/CasparTemplate.h \
        Models/CasparThumbnail.h \
        Shared.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

INCLUDEPATH += $$OUT_PWD/../Common $$PWD/../Common
DEPENDPATH += $$OUT_PWD/../Common $$PWD/../Common
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Common/release/ -lCommon
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Common/debug/ -lCommon
else:unix: LIBS += -L$$OUT_PWD/../Common/ -lCommon


