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

#include "deltadownloader.h"
#include "deltapatcher.h"
#include "model/cihash.h"
#include "model/settingsmodel.h"

class DeltaManager: public QObject
{
    Q_OBJECT

public:
    DeltaManager(const QStringList& deltaUrls, libtorrent::session* session,
                 QObject* parent = nullptr);
    ~DeltaManager() override;

    bool patchAvailable(const QString& modName);
    void patch(const QString& modName, const QString& key);
    void setDeltaUrls(const QStringList& deltaUrls);
    [[nodiscard]] QStringList deltaUrls() const;
    bool contains(const QString& key);
    [[nodiscard]] libtorrent::torrent_handle getHandle(const QString& url) const;
    boost::int64_t totalWanted(const QString& key);
    boost::int64_t totalWantedDone(const QString& key);
    QStringList folderKeys();
    [[nodiscard]] CiHash<QString> keyHash() const;
    //Helper function to get a mod name.
    QString name(const QString& key);
    [[nodiscard]] bool patchDownloading(const QString& key) const;
    bool patchExtracting(const QString& key);
    bool patching(const QString& key);
    bool queued(const QString& key);
    [[nodiscard]] const QList<libtorrent::torrent_handle> handles() const;
    [[nodiscard]] QString getUrl(const libtorrent::torrent_handle& handle) const;

public slots:
    void mirrorDeltaPatches();

signals:
    void patched(QString key, QString modName, bool success);

private slots:
    void handlePatched(const QString& modPath, bool success);
    void update();

private:
    CiHash<QString> keyHash_;
    DeltaDownloader downloader_;
    DeltaPatcher* patcher_{nullptr};
    QSet<QString> inDownload_;
    QThread patcherThread_;
    QTimer updateTimer_;
    SettingsModel& settings_{SettingsModel::instance()};

    [[nodiscard]] QString findDownloadedMod() const;
    static void removeDeprecatedPatches(const QStringList& urls);
};
#endif // DELTAMANAGER_H
