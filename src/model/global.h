#ifndef GLOBAL_H
#define GLOBAL_H
#include <QString>
#include <QTextStream>
#include "treemodel.h"
#include "version.h"
#include "apis/isync.h"
#include "logmanager.h"

namespace Constants
{
    const QString DEFAULT_USERNAME = "user";
    const QString DEFAULT_PASSWORD = "password";
    const unsigned DEFAULT_PORT = 41000;
    const QString VERSION_STRING = VERSION_CHARS;
    const QString DELTA_PATCHES_NAME = "afisync_patches";
    static const QString LOG_FILE = "afisync.log";
    //At least 100 max etas can be summed without overflow
    const int MAX_ETA = std::numeric_limits<int>::max()/100;
}

namespace Global
{
    extern QThread* workerThread;
    extern TreeModel* model;
    extern QTextStream* logStream;
    extern ISync* sync;
    extern LogManager* logManager;
    extern bool guiless;
}

#endif // GLOBAL_H
