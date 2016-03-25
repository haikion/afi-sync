#include <QTextStream>
#include "treemodel.h"
#include "global.h"

namespace Global
{
    QThread* workerThread = new QThread();
    TreeModel* model = nullptr;
    QTime* runningTime = new QTime();
    QTextStream* logStream = nullptr;
    BtsApi2* btsync = nullptr;
    bool guiless = false;
}
