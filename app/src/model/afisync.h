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
    QStringList filterDeprecatedPatches(const QStringList& allFiles, const QStringList& urls);
    QStringList getDeltaUrlsForMod(const QString& modName, const QString& hash, const QStringList& deltaUrls);
}

#endif // AFISYNC_H
