#include <QTextStream>
#include "treemodel.h"
#include "global.h"

namespace Global
{
    TreeModel* model = nullptr;
    QTextStream* logStream = nullptr;
    ISync* sync = nullptr;
    bool guiless = false;
}
