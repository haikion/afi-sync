/*
 * Manages Download of delta patches.
 */
#pragma once

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QString>
#include <QThread>

#ifdef Q_OS_WIN
#pragma warning(push, 0)
#endif
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/alert_types.hpp>
#ifdef Q_OS_WIN
#pragma warning(pop)
#endif

#include "model/cihash.h"

class DeltaDownloader: public QObject
{
    Q_OBJECT

public:
    DeltaDownloader();
    ~DeltaDownloader() override = default;

    void mirrorDeltaPatches();
    bool patchAvailable(const QString& modName);
    bool downloadPatches(const QString& modName, const QString& key);
    [[nodiscard]] libtorrent::torrent_handle getHandle(const QString& key) const;
    boost::int64_t totalWanted(const QString& modName);
    int64_t totalWantedDone(const QString& modName);
    [[nodiscard]] bool patchesDownloaded(const QString& key) const;
    void setDeltaUrls(const QStringList& deltaUrls);
    [[nodiscard]] QStringList deltaUrls() const;
    [[nodiscard]] const QList<libtorrent::torrent_handle> handles() const;
    [[nodiscard]] QString getUrl(const libtorrent::torrent_handle&) const;
    void setSession(libtorrent::session* newSession);

signals:
    void patchDlFailed();

private:
    QMap<QString, QList<libtorrent::torrent_handle>> handleMap_;
    CiHash<libtorrent::torrent_handle> urlHandleMap_;
    QStringList inDl_;
    QHash<QString, QVector<int>> fileIndexCache_;
    QNetworkAccessManager nam_;
    QStringList deltaUrls_;
    QStringList mirrored_;
    libtorrent::session* session_{nullptr};

    void addToHandleMap(const QString& key, const libtorrent::torrent_handle& torrentHandle);
    [[nodiscard]] QStringList getDeltaUrls(const QString& modName) const;
    [[nodiscard]] static QString hash(const QString& modName);
};
