TEMPLATE = app

QT += qml quick
CONFIG += c++11

RC_ICONS = src/view/armafin-logo-64px2.ico


win32 {
    INCLUDEPATH += D:\AfiSync\sources\libtorrent-rasterbar-1.1.0\include
    INCLUDEPATH += D:\AfiSync\sources\boost_1_61_0
    LIBS += -LD:\AfiSync\sources\boost_1_61_0\stage\lib -llibboost_system-mgw49-mt-1_61 -lws2_32 #  -llibboost_chrono-mgw49-mt-1_61
    LIBS += -LD:\AfiSync\sources\libtorrent-rasterbar-1.1.0\bin\gcc-mingw-4.9.2\release\threading-multi -llibtorrent.dll
}

unix {
    LIBS += -ltorrent-rasterbar -lboost_system
}

DEFINES += _HAS_ITERATOR_DEBUGGING=0 _SECURE_SCL=0
#DEFINES += BOOST_USE_WINAPI_VERSION=0x0501

SOURCES += src/model/main.cpp \
    src/model/treeitem.cpp \
    src/model/treemodel.cpp \
    src/model/rootitem.cpp \
    src/model/apis/btsync/btsapi2.cpp \
    src/model/syncitem.cpp \
    src/model/mod.cpp \
    src/model/repository.cpp \
    src/model/jsonreader.cpp \
    src/model/installer.cpp \
    src/model/apis/btsync/libbtsync-qt/bts_api.cpp \
    src/model/apis/btsync/libbtsync-qt/bts_apifolder.cpp \
    src/model/apis/btsync/libbtsync-qt/bts_client.cpp \
    src/model/apis/btsync/libbtsync-qt/bts_global.cpp \
    src/model/apis/btsync/libbtsync-qt/bts_remoteclient.cpp \
    src/model/apis/btsync/libbtsync-qt/bts_spawnclient.cpp \
    src/model/pathfinder.cpp \
    src/model/settingsmodel.cpp \
    src/model/global.cpp \
    src/model/modadapter.cpp \
    src/model/apis/heart.cpp \
    src/model/runningtime.cpp \
    src/model/processmonitor.cpp \
    src/model/apis/libtorrent/libtorrentapi.cpp \
    src/model/syncnetworkaccessmanager.cpp \
    src/model/apis/libtorrent/speedestimator.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    src/model/treeitem.h \
    src/model/treemodel.h \
    src/model/rootitem.h \
    src/model/apis/btsync/btsapi2.h \
    src/model/syncitem.h \
    src/model/apis/btsync/btsfolderactivity.h \
    src/model/apis/btsync/apikey.h \
    src/model/repository.h \
    src/model/mod.h \
    src/model/apis/btsync/libbtsync-qt/bts_api.h \
    src/model/apis/btsync/libbtsync-qt/bts_apifolder.h \
    src/model/apis/btsync/libbtsync-qt/bts_client.h \
    src/model/apis/btsync/libbtsync-qt/bts_export_helper.h \
    src/model/apis/btsync/libbtsync-qt/bts_global.h \
    src/model/apis/btsync/libbtsync-qt/bts_remoteclient.h \
    src/model/apis/btsync/libbtsync-qt/bts_spawnclient.h \
    src/model/jsonreader.h \
    src/model/installer.h \
    src/model/pathfinder.h \
    src/model/settingsmodel.h \
    src/model/global.h \
    src/model/debug.h \
    src/model/modadapter.h \
    src/model/apis/heart.h \
    src/model/runningtime.h \
    src/model/apis/isync.h \
    src/model/processmonitor.h \
    src/model/apis/libtorrent/libtorrentapi.h \
    src/model/cihash.h \
    src/model/syncnetworkaccessmanager.h \
    src/model/apis/libtorrent/speedestimator.h
DISTFILES +=
