TEMPLATE = app

QT += qml quick widgets
CONFIG += c++14

win32 {
    DEFINES += _WIN32_WINNT=0x0501
    INCLUDEPATH += ..\src\libtorrent-rasterbar-1.1.1\include
    INCLUDEPATH += ..\src\boost_1_63_0
    RC_ICONS = src/view/armafin-logo-64px2.ico
    LIBS += -L..\lib
    !Release {
        #Dynamic build
        DEFINES += BOOST_ALL_NO_LIB
        LIBS += -lboost_system-vc140-mt-gd-1_63 -lboost_atomic-vc140-mt-gd-1_63 -lboost_random-vc140-mt-gd-1_63 -lws2_32 -ltorrent
    }
    Release {
        #Static build
        LIBS += -llibboost_system-vc140-mt-s-1_63 -llibboost_atomic-vc140-mt-s-1_63 -llibboost_random-vc140-mt-s-1_63 -lws2_32 -llibtorrent
        DEFINES += STATIC_BUILD=1
        #Ask administrator permissions during startup
        !console: QMAKE_POST_LINK += mt.exe -manifest $$PWD/manifest.xml -outputresource:$$OUT_PWD/release/$${TARGET}.exe
    }
}

unix {
    LIBS += -ltorrent-rasterbar -lboost_system
}

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
    src/model/fileutils.cpp \
    src/model/logmanager.cpp \
    src/model/console.cpp \
    src/model/szip.cpp

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
    src/model/fileutils.h \
    src/model/logmanager.h \
    src/model/console.h \
    src/model/szip.h

DISTFILES += \
    AFISync.rc \
    manifest.xml
