#include "constantsmodel.h"
#include "global.h"

ConstantsModel::ConstantsModel(QObject* parent):
    QObject(parent)
{
}

QString ConstantsModel::defaultPort()
{
    return Constants::DEFAULT_PORT;
}
