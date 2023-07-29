#-------------------------------------------------
#
# Project created by QtCreator 2016-08-08T12:24:53
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = system-monitor
TEMPLATE = app

target.path = /usr/bin

desktop.path = /usr/share/applications
desktop.files += system-monitor.desktop

INSTALLS += target desktop

#icon.path = utilities-system-monitor
#icon.files += system-monitor.png

VERSION_MAJOR = 0
VERSION_MINOR = 0
VERSION_BUILD = 121

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR"\
       "VERSION_MINOR=$$VERSION_MINOR"\
       "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}

SOURCES += main.cpp\
    workerthread.cpp \
    resourcesworker.cpp \
    filesystemworker.cpp \
    qcustomplot.cpp \
    cputools.cpp \
    memoryconverter.cpp \
    colourhelper.cpp \
    rcmonitor.cpp

HEADERS  += \
    tablenumberitem.h \
    workerthread.h \
    resourcesworker.h \
    tablememoryitem.h \
    filesystemworker.h \
    qcustomplot.h \
    cputools.h \
    hashqstring.h \
    memoryconverter.h \
    colourhelper.h \
    rcmonitor.h

FORMS    += \
    rcmonitor.ui

QMAKE_CXXFLAGS += -std=c++14 -Wall
LIBS += -L"libprocps6" -lprocps -lstdc++fs

DISTFILES += \
    system-monitor.desktop
