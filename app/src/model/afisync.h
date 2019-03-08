#ifndef AFISYNC_H
#define AFISYNC_H

#include <QSet>
#include "interfaces/irepository.h"

/**
 * General helper functions
 */
namespace AfiSync
{
    QStringList activeModNames(const QList<IRepository*>& repositories);
    QSet<QString> activeModNames(const IRepository* repository);
}

#endif // AFISYNC_H
