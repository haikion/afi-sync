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

/*
 0. piece is not downloaded at all
 1. normal priority. Download order is dependent on availability
 2. higher than normal priority. Pieces are preferred over pieces with the same availability, but not over pieces with lower availability
 3. pieces are as likely to be picked as partial pieces.
 4. pieces are preferred over partial pieces, but not over pieces with lower availability
 5  currently the same as 4
 6. piece is as likely to be picked as any piece with availability 1
 7. maximum priority, availability is disregarded, the piece is preferred over any other piece with lower priority
 */
enum DownloadPriority
{
    NO_DOWNLOAD = 0,
    NORMAL = 1
};

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
