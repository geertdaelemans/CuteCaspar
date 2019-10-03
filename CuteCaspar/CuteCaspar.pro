#-------------------------------------------------
#
# Project created by QtCreator 2019-07-31T11:14:41
#
#-------------------------------------------------

QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CuteCaspar
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        CasparOSCListener.cpp \
        DeviceDialog.cpp \
        Main.cpp \
        MidiConnection.cpp \
        MidiLogger.cpp \
        MidiReader.cpp \
        PlayListDialog.cpp \
        Player.cpp \
        SettingsDialog.cpp \
        ip/IpEndpointName.cpp \
        ip/NetworkingUtils.cpp \
        ip/UdpSocket.cpp \
        MainWindow.cpp \
        osc/OscOutboundPacketStream.cpp \
        osc/OscPrintReceivedElements.cpp \
        osc/OscReceivedElements.cpp \
        osc/OscTypes.cpp

HEADERS += \
        CasparOSCListener.h \
        DeviceDialog.h \
        MainWindow.h \
        MidiConnection.h \
        MidiLogger.h \
        MidiReader.h \
        Models/LibraryModel.h \
        PlayListDialog.h \
        Player.h \
        SettingsDialog.h \
        ip/IpEndpointName.h \
        ip/NetworkingUtils.h \
        ip/PacketListener.h \
        ip/TimerListener.h \
        ip/UdpSocket.h \
        osc/MessageMappingOscPacketListener.h \
        osc/OscException.h \
        osc/OscHostEndianness.h \
        osc/OscOutboundPacketStream.h \
        osc/OscPacketListener.h \
        osc/OscPrintReceivedElements.h \
        osc/OscReceivedElements.h \
        osc/OscTypes.h

FORMS += \
        DeviceDialog.ui \
        MainWindow.ui \
        PlayList.ui \
        SettingsDialog.ui

LIBS += -lws2_32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    cutecaspar.qrc \
    images.qrc

DISTFILES +=


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Common/release/ -lCommon
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Common/debug/ -lCommon
else:unix: LIBS += -L$$OUT_PWD/../Common/ -lCommon

INCLUDEPATH += $$OUT_PWD/../Common $$PWD/../Common
DEPENDPATH += $$OUT_PWD/../Common $$PWD/../Common


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Caspar/release/ -lCaspar
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Caspar/debug/ -lCaspar
else:unix: LIBS += -L$$OUT_PWD/../Caspar/ -lCaspar

INCLUDEPATH += $$PWD/../Caspar
DEPENDPATH += $$PWD/../Caspar


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Core/release/ -lCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Core/debug/ -lCore
else:unix: LIBS += -L$$OUT_PWD/../Core/ -lCore

INCLUDEPATH += $$PWD/../Core
DEPENDPATH += $$PWD/../Core

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QMidi/release/ -lQMidi
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QMidi/debug/ -lQMidi
else:unix: LIBS += -L$$OUT_PWD/../QMidi/ -lQMidi

INCLUDEPATH += $$PWD/../QMidi
DEPENDPATH += $$PWD/../QMidi
