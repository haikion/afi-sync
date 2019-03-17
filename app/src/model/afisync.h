#ifndef AFISYNC_H
#define AFISYNC_H

#include <QSet>
#include <QList>
#include "interfaces/irepository.h"
#include "deletabledetector.h"

/**
 * General helper functions
 */
namespace AfiSync
{
    QStringList activeModNames(const QList<IRepository*>& repositories);
    QSet<QString> activeModNames(const IRepository* repository);
    void printDeletables(const DeletableDetector& deletableDetector);
    QSet<QString> combine(const QList<QSet<QString>>& list);
}

#endif // AFISYNC_H
