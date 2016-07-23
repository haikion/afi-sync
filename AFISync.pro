TEMPLATE = app

QT += qml quick widgets
CONFIG += c++11

win32 {
    RC_FILE = AFISync.rc
    INCLUDEPATH += D:\AfiSync\sources\libtorrent-rasterbar-1.1.0\include
    INCLUDEPATH += D:\AfiSync\sources\boost_1_61_0
    LIBS += -LD:\AfiSync\sources\boost_1_61_0\stage\lib -llibboost_system-mgw49-mt-1_61 -lws2_32
    LIBS += -LD:\AfiSync\sources\libtorrent-rasterbar-1.1.0\bin\gcc-mingw-4.9.2\release\threading-multi -llibtorrent.dll
    #Remove warning from libTorrent src
    QMAKE_CXXFLAGS += -Wno-missing-field-initializers
}

unix {
    LIBS += -ltorrent-rasterbar -lboost_system
}

DEFINES += _HAS_ITERATOR_DEBUGGING=0 _SECURE_SCL=0

SOURCES += src/model/main.cpp \
    src/model/treeitem.cpp \
    src/model/treemodel.cpp \
    src/model/rootitem.cpp \
    src/model/syncitem.cpp \
    src/model/mod.cpp \
    src/model/repository.cpp \
    src/model/jsonreader.cpp \
    src/model/installer.cpp \
    src/model/pathfinder.cpp \
    src/model/settingsmodel.cpp \
    src/model/global.cpp \
    src/model/modadapter.cpp \
    src/model/apis/heart.cpp \
    src/model/runningtime.cpp \
    src/model/processmonitor.cpp \
    src/model/apis/libtorrent/libtorrentapi.cpp \
    src/model/syncnetworkaccessmanager.cpp \
    src/model/apis/libtorrent/speedestimator.cpp \
    src/model/apis/libtorrent/ahasher.cpp \
    src/model/apis/libtorrent/deltadownloader.cpp \
    src/model/apis/libtorrent/deltapatcher.cpp \
    src/model/apis/libtorrent/deltamanager.cpp \
    src/model/fileutils.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    src/model/treeitem.h \
    src/model/treemodel.h \
    src/model/rootitem.h \
    src/model/syncitem.h \
    src/model/repository.h \
    src/model/mod.h \
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
    src/model/apis/libtorrent/speedestimator.h \
    src/model/version.h \
    src/model/apis/libtorrent/ahasher.h \
    src/model/apis/libtorrent/deltadownloader.h \
    src/model/apis/libtorrent/deltapatcher.h \
    src/model/apis/libtorrent/deltamanager.h \
    src/model/fileutils.h
DISTFILES += \
    AFISync.rc \
    manifest.xml
