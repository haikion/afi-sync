#include <QTextStream>
#include <QThread>
#include "treemodel.h"
#include "global.h"

namespace Global
{
    QThread* workerThread = nullptr;
    TreeModel* model = nullptr;
    QTextStream* logStream = nullptr;
    ISync* sync = nullptr;
    LogManager* logManager = nullptr;
    bool guiless = false;
}
