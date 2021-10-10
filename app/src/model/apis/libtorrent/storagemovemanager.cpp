#include <QStringList>
#include "../../fileutils.h"
#include "storagemovemanager.h"

StorageMoveManager::StorageMoveManager():
    QObject(nullptr) // Deleted manually because of threading issues
{
}

bool StorageMoveManager::inactive()
{
    return dirWatchers_.isEmpty();
}

bool StorageMoveManager::contains(const QString& key)
{
    return dirWatchers_.contains(key);
}

void StorageMoveManager::update()
{
    QStringList removeThese;
    for (DirectoryWatcher& dirWatcher : dirWatchers_)
    {
        if (dirWatcher.done())
        {
            removeThese.append(dirWatcher.key());
        }
    }

    dirWatchers_.removeAll(removeThese);
}

void StorageMoveManager::insert(const QString& key, const QString& fromPath, const QString& toPath, const qint64 totalDownloaded)
{
    dirWatchers_.insert(key, DirectoryWatcher(fromPath, toPath, key, totalDownloaded));
}

qint64 StorageMoveManager::totalWanted(const QString& key)
{
    return dirWatchers_.value(key).totalWanted();
}

qint64 StorageMoveManager::totalWantedDone(const QString& key)
{
    return dirWatchers_.value(key).totalWantedDone();
}
