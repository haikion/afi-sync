#ifndef GLOBAL_H
#define GLOBAL_H
#include <QString>
#include <QThread>
#include <QTextStream>
#include "treemodel.h"
#include "version.h"
#include "apis/isync.h"

namespace Constants {
    const QString SETTINGS_PATH = "settings";
    const QString SYNC_SETTINGS_PATH = SETTINGS_PATH + "/sync";
    const QString DEFAULT_USERNAME = "user";
    const QString DEFAULT_PASSWORD = "password";
    const unsigned DEFAULT_PORT = 41000;
    const QString VERSION_STRING = VERSION_CHARS;
    const QString DELTA_PATCHES_NAME = "afisync_patches";
    static const QString LOG_FILE = "afisync.log";
}

namespace Global
{
    extern TreeModel* model;
    extern QTextStream* logStream;
    extern ISync* sync;
    extern bool guiless;
}

#endif // GLOBAL_H
