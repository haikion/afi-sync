#include <QTextStream>
#include "treemodel.h"
#include "global.h"

namespace Global
{
    QThread* workerThread = new QThread();
    TreeModel* model = nullptr;
    QTextStream* logStream = nullptr;
    BtsApi2* sync = nullptr;
    bool guiless = false;
}
