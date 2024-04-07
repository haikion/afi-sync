TEMPLATE = app

QT += core gui network widgets
CONFIG += c++17
TARGET = AFISync

win32 {
    DEFINES += _WIN32_WINNT=0x0501
    INCLUDEPATH += $$_PRO_FILE_PWD_\..\..\src\libtorrent-rasterbar-2.0.10\include
    INCLUDEPATH += $$_PRO_FILE_PWD_\..\..\src\boost_1_84_0
    RC_ICONS = src/view/armafin-logo-64px2.ico
    LIBS += -L$$_PRO_FILE_PWD_\..\..\lib
    !Release {
        QMAKE_CXXFLAGS += -wd4250
        DEFINES += BOOST_ALL_DYN_LINK
        DEFINES += TORRENT_LINKING_SHARED
        POSTFIX = vc143-mt-gd-x64-1_84
        LIBS += -lboost_log_setup-$${POSTFIX} -lboost_log-$${POSTFIX} -lboost_filesystem-$${POSTFIX} -lboost_system-$${POSTFIX} -lboost_date_time-$${POSTFIX} -lboost_thread-$${POSTFIX} -lboost_atomic-$${POSTFIX} -lboost_random-$${POSTFIX} -lws2_32 -ltorrent
    }
    Release {
        #Static build
        POSTFIX = vc143-mt-s-x64-1_84
        LIBS += -lbcrypt -llibboost_system-$${POSTFIX} -llibboost_atomic-$${POSTFIX} -llibboost_random-$${POSTFIX} -lws2_32 -llibtorrent-rasterbar
        DEFINES += STATIC_BUILD=1
        #Ask administrator permissions during startup
        !console: QMAKE_POST_LINK += mt.exe -manifest $$PWD/manifest.xml -outputresource:$$OUT_PWD/release/$${TARGET}.exe
    }
}

unix {
   QMAKE_CXX = ccache $$QMAKE_CXX
   DEFINES += BOOST_LOG_DYN_LINK
   QMAKE_CXXFLAGS += -Wno-unknown-pragmas
   LIBS += -lboost_system -lboost_atomic -lboost_random -lboost_date_time -lboost_log_setup -lboost_filesystem -lboost_log -lboost_thread -ltorrent-rasterbar
}

SOURCES += \
    ../deps/try_signal/signal_error_code.cpp \
    ../deps/try_signal/try_signal.cpp \
    src/model/main.cpp \
    src/model/destructionwaiter.cpp \
    src/model/afisync.cpp \
    src/model/deletable.cpp \
    src/model/treeitem.cpp \
    src/model/treemodel.cpp \
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
    src/model/apis/libtorrent/speedcalculator.cpp \
    src/model/syncnetworkaccessmanager.cpp \
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
    src/view/pathsetting.cpp \
    src/model/jsonutils.cpp \
    src/model/apis/libtorrent/alerthandler.cpp \
    src/model/apis/libtorrent/directorywatcher.cpp \
    src/model/apis/libtorrent/storagemovemanager.cpp

HEADERS += \
    ../deps/try_signal/signal_error_code.hpp \
    ../deps/try_signal/try_signal.hpp \
    src/model/destructionwaiter.h \
    src/model/afisync.h \
    src/model/deletable.h \
    src/model/treeitem.h \
    src/model/treemodel.h \
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
    src/model/apis/libtorrent/speedcalculator.h \
    src/model/cihash.h \
    src/model/syncnetworkaccessmanager.h \
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
    src/view/pathsetting.h \
    src/model/jsonutils.h \
    src/model/apis/libtorrent/alerthandler.h \
    src/model/apis/libtorrent/directorywatcher.h \
    src/model/apis/libtorrent/storagemovemanager.h \
    src/model/interfaces/imod.h \
    src/model/interfaces/imod.h
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
