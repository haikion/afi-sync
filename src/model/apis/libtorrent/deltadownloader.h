/*
 * Manages Download of delta patches.
 */
#ifndef DELTADOWNLOADER_H
#define DELTADOWNLOADER_H

#include <QString>
#include <QVector>
#include <QHash>
#include <QSet>
#include <QObject>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/alert_types.hpp>
#include "../../cihash.h"
#include "deltapatcher.h"

class DeltaDownloader: public QObject
{
    Q_OBJECT

public:
    DeltaDownloader(const libtorrent::torrent_handle& handle, QObject* parent = nullptr);

    bool patchAvailable(const QString& modName);
    bool patchDownloaded(const QString& modName);
    bool downloadPatch(const QString& modName);
    bool noPeers() const;
    void setPaused(bool value);
    libtorrent::torrent_handle handle();
    boost::int64_t totalWanted(const QString& modName);
    boost::int64_t totalWantedDone(const QString& modName);

private:
    libtorrent::torrent_handle handle_;
    libtorrent::file_storage fileStorage_;
    QStringList patches_;
    QHash<QString, QVector<int>> fileIndexCache_;

    QStringList patches(const QString& modName) const;
    void createFilePaths();
    QString hash(const QString& modName) const;
    QVector<int> patchIndexes(const QString& modName);
};

#endif // DELTADOWNLOADER_H
