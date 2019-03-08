include(gtest_dependency.pri)

TEMPLATE = app
QT += core gui network widgets
CONFIG += c++14
CONFIG -= app_bundle
CONFIG += thread

unix {
   DEFINES += BOOST_LOG_DYN_LINK
   QMAKE_CXXFLAGS += -Wno-unknown-pragmas
   LIBS += -lboost_system -lboost_atomic -lboost_random -lboost_date_time -lboost_log_setup -lboost_filesystem -lboost_log -lboost_thread -ltorrent-rasterbar
}

HEADERS += \
        tst_afisync.h \
    ../app/src/model/apis/libtorrent/ahasher.h \
    ../app/src/model/apis/libtorrent/alerthandler.h \
    ../app/src/model/apis/libtorrent/deltadownloader.h \
    ../app/src/model/apis/libtorrent/deltamanager.h \
    ../app/src/model/apis/libtorrent/deltapatcher.h \
    ../app/src/model/apis/libtorrent/directorywatcher.cpp.TtKWIM \
    ../app/src/model/apis/libtorrent/directorywatcher.h \
    ../app/src/model/apis/libtorrent/libtorrentapi.h \
    ../app/src/model/apis/libtorrent/speedestimator.h \
    ../app/src/model/apis/libtorrent/storagemovemanager.h \
    ../app/src/model/apis/isync.h \
    ../app/src/model/interfaces/ibandwidthmeter.h \
    ../app/src/model/interfaces/irepository.h \
    ../app/src/model/interfaces/isettings.h \
    ../app/src/model/interfaces/isyncitem.h \
    ../app/src/model/afisynclogger.h \
    ../app/src/model/cihash.h \
    ../app/src/model/console.h \
    ../app/src/model/constantsmodel.h \
    ../app/src/model/customdebug.h \
    ../app/src/model/deletabledetector.h \
    ../app/src/model/fileutils.h \
    ../app/src/model/global.h \
    ../app/src/model/installer.h \
    ../app/src/model/jsonreader.h \
    ../app/src/model/jsonutils.h \
    ../app/src/model/mod.h \
    ../app/src/model/modadapter.h \
    ../app/src/model/pathfinder.h \
    ../app/src/model/processmonitor.h \
    ../app/src/model/qstreams.h \
    ../app/src/model/repository.h \
    ../app/src/model/runningtime.h \
    ../app/src/model/settingsmodel.h \
    ../app/src/model/settingsuimodel.h \
    ../app/src/model/syncitem.h \
    ../app/src/model/syncnetworkaccessmanager.h \
    ../app/src/model/szip.h \
    ../app/src/model/treeitem.h \
    ../app/src/model/treemodel.h \
    ../app/src/model/version.h \
    ../app/src/model/deletable.h \
    ../app/src/model/afisync.h

SOURCES += \
        main.cpp \
    ../app/src/model/apis/libtorrent/ahasher.cpp \
    ../app/src/model/apis/libtorrent/alerthandler.cpp \
    ../app/src/model/apis/libtorrent/deltadownloader.cpp \
    ../app/src/model/apis/libtorrent/deltamanager.cpp \
    ../app/src/model/apis/libtorrent/deltapatcher.cpp \
    ../app/src/model/apis/libtorrent/directorywatcher.cpp \
    ../app/src/model/apis/libtorrent/libtorrentapi.cpp \
    ../app/src/model/apis/libtorrent/speedestimator.cpp \
    ../app/src/model/apis/libtorrent/storagemovemanager.cpp \
    ../app/src/model/afisynclogger.cpp \
    ../app/src/model/console.cpp \
    ../app/src/model/constantsmodel.cpp \
    ../app/src/model/deletabledetector.cpp \
    ../app/src/model/fileutils.cpp \
    ../app/src/model/global.cpp \
    ../app/src/model/installer.cpp \
    ../app/src/model/jsonreader.cpp \
    ../app/src/model/jsonutils.cpp \
    ../app/src/model/mod.cpp \
    ../app/src/model/modadapter.cpp \
    ../app/src/model/pathfinder.cpp \
    ../app/src/model/processmonitor.cpp \
    ../app/src/model/qstreams.cpp \
    ../app/src/model/repository.cpp \
    ../app/src/model/runningtime.cpp \
    ../app/src/model/settingsmodel.cpp \
    ../app/src/model/settingsuimodel.cpp \
    ../app/src/model/syncitem.cpp \
    ../app/src/model/syncnetworkaccessmanager.cpp \
    ../app/src/model/szip.cpp \
    ../app/src/model/treeitem.cpp \
    ../app/src/model/treemodel.cpp \
    deletabledetectortest.cpp \
    ../app/src/model/deletable.cpp \
    ../app/src/model/afisync.cpp
