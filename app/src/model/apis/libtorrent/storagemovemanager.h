#ifndef STORAGEMOVEMANAGER_H
#define STORAGEMOVEMANAGER_H

#include <QObject>
#include <QTimer>
#include "../../cihash.h"
#include "directorywatcher.h"

class StorageMoveManager : public QObject
{
    Q_OBJECT

public slots:
    void update();

public:
    StorageMoveManager();
    bool contains(const QString& key);
    void insert(const QString& key, const QString& fromPath, const QString& toPath);
    qint64 totalWanted(const QString& key);
    qint64 totalWantedDone(const QString& key);

private:
    CiHash<DirectoryWatcher> dirWatchers_;
};

#endif // STORAGEMOVEMANAGER_H
