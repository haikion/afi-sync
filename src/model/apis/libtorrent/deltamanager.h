/*
 * Encapsulates delta updating.
 */

#ifndef DELTAMANAGER_H
#define DELTAMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>

#include "libtorrent/torrent_handle.hpp"

#include "../../cihash.h"
#include "deltapatcher.h"
#include "deltadownloader.h"

class DeltaManager: public QObject
{
    Q_OBJECT

public:
    DeltaManager(libtorrent::torrent_handle handle, QObject* parent = nullptr);
    virtual ~DeltaManager();

    bool patchAvailable(const QString& modName);
    bool patch(const QString& modName, const QString& key);
    bool contains(const QString& key);
    libtorrent::torrent_handle handle() const;
    boost::int64_t totalWanted(const QString& key);
    boost::int64_t totalWantedDone(const QString& key);
    QList<QString> folderKeys();

signals:
    void patched(QString key, QString modName, bool success);

private slots:
    void handlePatched(const QString& modPath, bool success);
    void update();

private:
    DeltaDownloader* downloader_;
    DeltaPatcher* patcher_;
    CiHash<QString> keyHash_;
    libtorrent::torrent_handle handle_;
    QTimer* updateTimer_;
};

#endif // DELTAMANAGER_H