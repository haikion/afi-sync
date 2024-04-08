/*
 * Encapsulates delta updating.
 */

#ifndef DELTAMANAGER_H
#define DELTAMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>

#ifdef Q_OS_WIN
#pragma warning(push, 0)
#endif
#include "libtorrent/torrent_handle.hpp"
#ifdef Q_OS_WIN
#pragma warning(pop)
#endif

#include "../../cihash.h"
#include "deltapatcher.h"
#include "deltadownloader.h"

class DeltaManager: public QObject
{
    Q_OBJECT

public:
    DeltaManager(const QStringList& deltaUrls, libtorrent::session* session,
                 QObject* parent = nullptr);
    ~DeltaManager();

    bool patchAvailable(const QString& modName);
    void patch(const QString& modName, const QString& key);
    void setDeltaUrls(const QStringList& deltaUrls);
    QStringList deltaUrls() const;
    bool contains(const QString& key);
    libtorrent::torrent_handle getHandle(const QString& url) const;
    boost::int64_t totalWanted(const QString& key);
    boost::int64_t totalWantedDone(const QString& key);
    QStringList folderKeys();
    CiHash<QString> keyHash() const;
    //Helper function to get a mod name.
    QString name(const QString& key);
    bool patchDownloading(const QString& key) const;
    bool patchExtracting(const QString& key);
    bool patching(const QString& key);
    bool queued(const QString& key);
    const QList<libtorrent::torrent_handle> handles() const;
    QString getUrl(const libtorrent::torrent_handle& handle) const;

public slots:
    void mirrorDeltaPatches();

signals:
    void patched(QString key, QString modName, bool success);

private slots:
    void handlePatched(const QString& modPath, bool success);
    void update();

private:
    DeltaDownloader downloader_;
    DeltaPatcher* patcher_{nullptr};
    CiHash<QString> keyHash_;
    QSet<QString> inDownload_;
    QTimer updateTimer_;
    QThread patcherThread_;

    QSet<QString> torrentFilesUpper(const QString &url);
    QString findDownloadedMod() const;
    void removeDeprecatedPatches(const QStringList& urls);
};
#endif // DELTAMANAGER_H
