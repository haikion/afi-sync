TEMPLATE = app

QT += core gui network widgets
CONFIG += c++14
TARGET = AFISync

win32 {
    #Google Crashpad crash reporting
    include(src\model\crashhandler\crashhandler.pri)
    DEFINES += _WIN32_WINNT=0x0501
    INCLUDEPATH += ..\src\libtorrent-rasterbar-1.1.7\include
    INCLUDEPATH += ..\src\boost_1_66_0
    RC_ICONS = src/view/armafin-logo-64px2.ico
    LIBS += -L..\lib
    !Release {
        QMAKE_CXXFLAGS += -wd4250
        DEFINES += BOOST_ALL_DYN_LINK
        POSTFIX = vc140-mt-gd-x64-1_66
        LIBS += -lboost_log_setup-$${POSTFIX} -lboost_log-$${POSTFIX} -lboost_filesystem-$${POSTFIX} -lboost_system-$${POSTFIX} -lboost_date_time-$${POSTFIX} -lboost_thread-$${POSTFIX} -lboost_regex-$${POSTFIX} -lboost_atomic-$${POSTFIX} -lboost_random-$${POSTFIX} -lws2_32 -ltorrent
    }
    Release {
        #Static build
        LIBS += -llibboost_system-vc141-mt-s-x64-1_66 -llibboost_atomic-vc141-mt-s-x64-1_66 -llibboost_random-vc141-mt-s-x64-1_66 -lws2_32 -llibtorrent
        DEFINES += STATIC_BUILD=1
        #Generate pdb debug symbols for crash dumps
        QMAKE_CXXFLAGS+=/Zi
        QMAKE_LFLAGS+= /INCREMENTAL:NO /Debug
        #Ask administrator permissions during startup
        !console: QMAKE_POST_LINK += mt.exe -manifest $$PWD/manifest.xml -outputresource:$$OUT_PWD/release/$${TARGET}.exe
    }
}

unix {
   DEFINES += BOOST_LOG_DYN_LINK
   QMAKE_CXXFLAGS += -Wno-unknown-pragmas
   LIBS += -lboost_system -lboost_atomic -lboost_random -lboost_date_time -lboost_log_setup -lboost_filesystem -lboost_log -lboost_thread -ltorrent-rasterbar
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
    src/model/afisynclogger.cpp \
    src/model/console.cpp \
    src/model/szip.cpp \
    src/model/constantsmodel.cpp \
    src/model/qstreams.cpp \
    src/model/settingsuimodel.cpp \
    src/model/deletabledetector.cpp \
    src/view/asbannerbar.cpp \
    src/view/assettingsview.cpp \
    src/view/astreeitem.cpp \
    src/view/astreewidget.cpp \
    src/view/mainwindow.cpp \
    src/view/optionalsetting.cpp \
    src/view/pathsetting.cpp

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
    src/model/modadapter.h \
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
    src/model/afisynclogger.h \
    src/model/console.h \
    src/model/szip.h \
    src/model/constantsmodel.h \
    src/model/qstreams.h \
    src/model/interfaces/ibandwidthmeter.h \
    src/model/interfaces/irepository.h \
    src/model/interfaces/isettings.h \
    src/model/interfaces/isyncitem.h \
    src/model/settingsuimodel.h \
    src/model/deletabledetector.h \
    src/view/asbannerbar.h \
    src/view/assettingsview.h \
    src/view/astreeitem.h \
    src/view/astreewidget.h \
    src/view/mainwindow.h \
    src/view/optionalsetting.h \
    src/view/pathsetting.h

DISTFILES += \
    AFISync.rc \
    manifest.xml

FORMS += \
    src/view/asbannerbar.ui \
    src/view/assettingsview.ui \
    src/view/mainwindow.ui \
    src/view/optionalsetting.ui \
    src/view/pathsetting.ui

RESOURCES += \
    src/view/resources.qrc
