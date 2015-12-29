QT += gui network
TARGET = linkapp
TEMPLATE = app
include(./qt-single-application/qtsingleapplication.pri)

INCLUDEPATH +=../linkapp-build-desktop
CONFIG += release
LIBS += -lpcap -lnet
SOURCES += main.cpp mainwindow.cpp linkthread.cpp md5.cpp
HEADERS += mainwindow.h linkthread.h global.h md5.h \
    eap-header.h
FORMS += mainwindow.ui
RESOURCES += resources.qrc
