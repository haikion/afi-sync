#-------------------------------------------------
#
# Project created by QtCreator 2016-06-09T22:46:57
#
#-------------------------------------------------

QT += core gui network widgets testlib

CONFIG += c++14

win32 {
    DEFINES += _WIN32_WINNT=0x0501
    INCLUDEPATH += ..\..\src\libtorrent-rasterbar-1.1.2\include
    INCLUDEPATH += ..\..\src\boost_1_63_0
    #Dynamic build
    DEFINES += BOOST_ALL_NO_LIB
    LIBS += -L..\..\lib -lboost_system-vc140-mt-gd-1_63 -lboost_atomic-vc140-mt-gd-1_63 -lboost_random-vc140-mt-gd-1_63 -lws2_32 -ltorrent
}

unix {
   DEFINES += BOOST_LOG_DYN_LINK
   LIBS += -lboost_system -lboost_atomic -lboost_random -lboost_date_time -lboost_log_setup -lboost_filesystem -lboost_log -lboost_thread -ltorrent-rasterbar
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
    ../src/model/fileutils.cpp \
    ../src/model/console.cpp \
    ../src/model/szip.cpp \
    ../src/model/settingsuimodel.cpp \
    ../src/model/qstreams.cpp \
    ../src/model/constantsmodel.cpp \
    ../src/model/afisynclogger.cpp \
    ../src/model/deletabledetector.cpp

DEFINES += SRCDIR=\\\"$$PWD/\\\"

HEADERS += \
    ../src/model/cihash.h \
    ../src/model/customdebug.h \
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
    ../src/model/fileutils.h \
    ../src/model/console.h \
    ../src/model/version.h \
    ../src/model/szip.h \
    ../src/model/settingsuimodel.h \
    ../src/model/qstreams.h \
    ../src/model/constantsmodel.h \
    ../src/model/afisynclogger.h \
    ../src/model/deletabledetector.cpp
