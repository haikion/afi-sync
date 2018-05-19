#include <QTextStream>
#include <QThread>
#include "treemodel.h"
#include "global.h"

namespace Global
{
    ISync* sync = nullptr;
    QTextStream* logStream = nullptr;
    QThread* workerThread = nullptr;
    TreeModel* model = nullptr;
    bool guiless = false;
}
