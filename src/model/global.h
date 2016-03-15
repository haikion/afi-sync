#ifndef GLOBAL_H
#define GLOBAL_H
#include <QString>
#include "treemodel.h"
#include "apis/btsync/btsapi2.h"

namespace Constants {
    const QString SETTINGS_PATH = "settings";
    const QString BTSYNC_SETTINGS_PATH = SETTINGS_PATH + "/BitTorrent_Sync";
    const QString DEFAULT_USERNAME = "user";
    const QString DEFAULT_PASSWORD = "password";
    const unsigned DEFAULT_PORT = 8887;
    const QString VERSION_STRING = "v0.27alpha";
}

namespace Global
{
    extern TreeModel* model;
    extern QTime* runningTime;
    extern QTextStream* logStream;
    extern BtsApi2* btsync;
    extern bool guiless;
}

#endif // GLOBAL_H
