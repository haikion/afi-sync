#ifndef GLOBAL_H
#define GLOBAL_H

#include <QString>
#include <QTextStream>

#include "apis/isync.h"
#include "treemodel.h"

namespace Constants
{
    static const QString DEFAULT_PORT = QStringLiteral("41000");
    static const QString LOG_FILE = QStringLiteral("afisync.log");
    static constexpr qint64 MEGA_DIVIDER = 1024 * 1024;
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
