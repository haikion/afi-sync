TEMPLATE = app

QT += qml quick
CONFIG += c++11

INCLUDEPATH += /usr/local/include/libbtsync-qt
RC_ICONS = armafin-logo-64px2.ico
#LIBS += -L/usr/local/lib -lbtsync-qt
#DEFINES += QT_NO_DEBUG_OUTPUT

SOURCES += src/model/main.cpp \
    src/model/treeitem.cpp \
    src/model/treemodel.cpp \
    src/model/rootitem.cpp \
    src/model/api/btsapi2.cpp \
    src/model/syncitem.cpp \
    src/model/mod.cpp \
    src/model/repository.cpp \
    src/model/jsonreader.cpp \
    src/model/installer.cpp \
    src/model/api/libbtsync-qt/bts_api.cpp \
    src/model/api/libbtsync-qt/bts_apifolder.cpp \
    src/model/api/libbtsync-qt/bts_client.cpp \
    src/model/api/libbtsync-qt/bts_global.cpp \
    src/model/api/libbtsync-qt/bts_remoteclient.cpp \
    src/model/api/libbtsync-qt/bts_spawnclient.cpp \
    src/model/pathfinder.cpp \
    src/model/settingsmodel.cpp \
    src/model/global.cpp \
    src/model/debug.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

HEADERS += \
    src/model/treeitem.h \
    src/model/treemodel.h \
    src/model/rootitem.h \
    src/model/api/btsapi2.h \
    src/model/syncitem.h \
    src/model/api/btsfolderactivity.h \
    src/model/apikey.h \
    src/model/repository.h \
    src/model/mod.h \
    src/model/api/libbtsync-qt/bts_api.h \
    src/model/api/libbtsync-qt/bts_apifolder.h \
    src/model/api/libbtsync-qt/bts_client.h \
    src/model/api/libbtsync-qt/bts_export_helper.h \
    src/model/api/libbtsync-qt/bts_global.h \
    src/model/api/libbtsync-qt/bts_remoteclient.h \
    src/model/api/libbtsync-qt/bts_spawnclient.h \
    src/model/jsonreader.h \
    src/model/installer.h \
    src/model/pathfinder.h \
    src/model/settingsmodel.h \
    src/model/global.h \
    src/model/debug.h

DISTFILES +=
