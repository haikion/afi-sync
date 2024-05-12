#ifndef AFISYNC_H
#define AFISYNC_H

#include <QList>
#include <QSet>
#include <QSharedPointer>

#include "interfaces/irepository.h"
#include "deletabledetector.h"

class Repository;
using RepositoryList = QList<QSharedPointer<Repository>>;

/**
 * General helper functions
 */
namespace AfiSync
{
    QStringList activeModNames(const QList<IRepository*>& repositories);
    [[nodiscard]] QSharedPointer<Repository> findRepoByName(const QString& name, const RepositoryList& repositories);
    [[nodiscard]] const QSet<QString> activeModNames(const IRepository* repository);
    void printDeletables(const DeletableDetector& deletableDetector);
    [[nodiscard]] QStringList filterDeprecatedPatches(const QStringList& allFiles, const QStringList& urls);
    [[nodiscard]] QStringList getDeltaUrlsForMod(const QString& modName, const QString& hash, const QStringList& deltaUrls);
}

#endif // AFISYNC_H
