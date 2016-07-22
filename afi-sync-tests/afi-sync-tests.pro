#-------------------------------------------------
#
# Project created by QtCreator 2016-06-09T22:46:57
#
#-------------------------------------------------

QT += qml quick testlib

CONFIG += c++11

win32 {
    INCLUDEPATH += D:\AfiSync\sources\libtorrent-rasterbar-1.1.0\include
    INCLUDEPATH += D:\AfiSync\sources\boost_1_61_0
    LIBS += -LD:\AfiSync\sources\boost_1_61_0\stage\lib -llibboost_system-mgw49-mt-1_61 -lws2_32 #  -llibboost_chrono-mgw49-mt-1_61
    LIBS += -LD:\AfiSync\sources\libtorrent-rasterbar-1.1.0\bin\gcc-mingw-4.9.2\release\threading-multi -llibtorrent.dll
}

unix {
    LIBS += -ltorrent-rasterbar -lboost_system
}

TARGET = tst_afisynctest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += tst_afisynctest.cpp \
    ../src/model/global.cpp \
    ../src/model/installer.cpp \
    ../src/model/jsonreader.cpp \
    ../src/model/mod.cpp \
    ../src/model/modadapter.cpp \
    ../src/model/pathfinder.cpp \
    ../src/model/processmonitor.cpp \
    ../src/model/repository.cpp \
    ../src/model/rootitem.cpp \
    ../src/model/runningtime.cpp \
    ../src/model/settingsmodel.cpp \
    ../src/model/syncitem.cpp \
    ../src/model/treeitem.cpp \
    ../src/model/treemodel.cpp \
    ../src/model/apis/libtorrent/libtorrentapi.cpp \
    ../src/model/syncnetworkaccessmanager.cpp \
    ../src/model/apis/libtorrent/speedestimator.cpp \
    ../src/model/apis/libtorrent/ahasher.cpp \
    ../src/model/apis/libtorrent/deltadownloader.cpp \
    ../src/model/apis/libtorrent/deltamanager.cpp \
    ../src/model/apis/libtorrent/deltapatcher.cpp \
    ../src/model/fileutils.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
    ../src/model/cihash.h \
    ../src/model/customdebug.h \
    ../src/model/debug.h \
    ../src/model/global.h \
    ../src/model/installer.h \
    ../src/model/jsonreader.h \
    ../src/model/mod.h \
    ../src/model/modadapter.h \
    ../src/model/pathfinder.h \
    ../src/model/processmonitor.h \
    ../src/model/repository.h \
    ../src/model/rootitem.h \
    ../src/model/runningtime.h \
    ../src/model/settingsmodel.h \
    ../src/model/syncitem.h \
    ../src/model/treeitem.h \
    ../src/model/treemodel.h \
    ../src/model/apis/libtorrent/libtorrentapi.h \
    ../src/model/apis/isync.h \
    ../src/model/syncnetworkaccessmanager.h \
    ../src/model/apis/libtorrent/speedestimator.h \
    ../src/model/apis/libtorrent/ahasher.h \
    ../src/model/apis/libtorrent/deltadownloader.h \
    ../src/model/apis/libtorrent/deltamanager.h \
    ../src/model/apis/libtorrent/deltapatcher.h \
    ../src/model/fileutils.h
