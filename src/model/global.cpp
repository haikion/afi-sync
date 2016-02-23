#include <QTextStream>
#include "treemodel.h"
#include "global.h"

namespace Global
{
    TreeModel* model = nullptr;
    QTime* runningTime = new QTime();
    QTextStream* logStream = nullptr;
    BtsApi2* btsync = nullptr;
    bool guiless = false;
}
