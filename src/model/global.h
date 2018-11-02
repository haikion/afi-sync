#ifndef GLOBAL_H
#define GLOBAL_H
#include <QString>
#include <QTextStream>
#include "treemodel.h"
#include "version.h"
#include "apis/isync.h"
#include "afisynclogger.h"

namespace Constants
{
    static const QString DEFAULT_PASSWORD = "password";
    static const QString DEFAULT_PORT = "41000";
    static const QString DEFAULT_USERNAME = "user";
    static const QString DELTA_PATCHES_NAME = "afisync_patches";
    static const QString LOG_FILE = "afisync.log";
    static const QString VERSION_STRING = VERSION_CHARS;
    static const int MAX_ETA = std::numeric_limits<int>::max()/100;
    static const qint64 MILLION = 1000000;
}

namespace Global
{
    extern ISync* sync;
    extern QTextStream* logStream;
    extern QThread* workerThread;
    extern TreeModel* model;
    extern bool guiless;
}

#endif // GLOBAL_H
