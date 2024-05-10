#ifndef LIBTORRENTAPI_H
#define LIBTORRENTAPI_H

#include <QDir>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QTimer>

#ifdef Q_OS_WIN
#pragma warning(push, 0)
#endif
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/alert.hpp>
#ifdef Q_OS_WIN
#pragma warning(pop)
#endif

#include "../../cihash.h"
#include "../isync.h"
#include "alerthandler.h"
#include "deltamanager.h"
#include "storagemovemanager.h"
#include "model/settingsmodel.h"

class LibTorrentApi : public QObject, virtual public ISync
{
    Q_OBJECT
    Q_INTERFACES(ISync)

public:
    explicit LibTorrentApi();
    explicit LibTorrentApi(const QStringList& deltaUrls);
    ~LibTorrentApi() override;

    void mirrorDeltaPatches() override;
    QStringList deltaUrls() override;
    bool disableDeltaUpdates() override;
    void disableDeltaUpdatesNoTorrents();
    void enableDeltaUpdates() override;

    void checkFolder(const QString& key) override;
    //Returns true if there are no peers.
    bool folderNoPeers(const QString& key) override;
    //Returns list of keys of added folders.
    QStringList folderKeys() override;
    bool folderReady(const QString& key) override;
    //Returns true if folder is indexing or checking files.
    bool folderChecking(const QString& key) override;
    bool folderDownloading(const QString& key) override;
    bool folderMovingFiles(const QString& key) override;
    bool folderQueued(const QString& key) override;
    bool folderPatching(const QString& key) override;
    //Adds folder, path is local system directory, key is source.
    bool addFolder(const QString& key, const QString& name) override;
    //Removes folder with specific key.
    void removeFolder(const QString& key) override;
    //Returns list of files in folder in upper case format.
    QSet<QString> folderFilesUpper(const QString& key) override;
    //Returns true if folder with specific key exists.
    bool folderExists(const QString& key) override;
    void setDeltaUrls(const QStringList& urls) override;
    void setFolderPath(const QString& key, const QString& path) override;
    //Returns true if folder is paused
    bool folderPaused(const QString& key) override;
    //Sets folder in paused mode or starts if if value is set to false.
    void setFolderPaused(const QString& key, bool value) override;
    //Returns string describing the error. Returns empty string if no error.
    QString folderError(const QString& key) override;
    //Returns file system path of the folder with specific key.
    QString folderPath(const QString& key) override;
    //Total bandwiths
    [[nodiscard]] int64_t download() const override;
    int64_t upload() override;
    //Sets global max upload
    void setMaxUpload(int limit) override;
    //Sets global max download
    void setMaxDownload(int limit) override;
    //Returns true if the sync has loaded and is ready to take commands.
    bool ready() override;
    //Sets outgoing port.
    void setPort(int port) override;
    bool folderDownloadingPatches(const QString& key) override;
    void disableQueue(const QString& key) override;
    [[nodiscard]] int64_t folderFileSize(const QString& key) override;
    [[nodiscard]] int64_t folderTotalWanted(const QString& key) override;
    [[nodiscard]] int64_t folderTotalWantedDone(const QString& key) override;
    void cleanUnusedFiles(const QSet<QString>& usedKeys) override;
    bool folderExtractingPatch(const QString& key) override;
    bool folderCheckingPatches(const QString& key) override;

    static QPair<libtorrent::error_code, libtorrent::add_torrent_params> toAddTorrentParams(const QByteArray& torrentData);

private slots:
    void handleAlerts();
    void handlePatched(const QString& key, const QString& modName, bool success);
    void init();
    void initDelta();

signals:
    void initCompleted();
    void shutdownCompleted();
    void folderAdded(QString key);

private:
    static const int NOT_FOUND;

    QTimer* timer_{nullptr};
    QNetworkAccessManager* networkAccessManager_{nullptr};
    libtorrent::session* session_{nullptr};
    // TODO: Create single type called TorrentHandle
    // and combine these.
    CiHash<libtorrent::torrent_handle> keyHash_;
    CiHash<libtorrent::add_torrent_params> torrentParams_;
    CiHash<QString> prefixMap_;
    QSet<QString> torrentDownloading_;

    AlertHandler* alertHandler_{nullptr};
    DeltaManager* deltaManager_{nullptr};
    SettingsModel& settings_{SettingsModel::instance()};
    bool creatingDeltaManager_{false};
    QQueue<QPair<QString, QString>> pendingFolder_;
    QStringList deltaUrls_;
    QString settingsPath_;
    StorageMoveManager* storageMoveManager_{nullptr};
    libtorrent::time_point statsLastTimestamp_;
    int64_t downloadSpeed_{0};
    int64_t uploadSpeed_{0};
    std::atomic<bool> ready_{false};

    [[nodiscard]] static std::shared_ptr<libtorrent::torrent_info> loadFromFile(const QString& path);
    void loadTorrentFiles(const QDir& dir);
    void saveTorrentFile(const libtorrent::torrent_handle& getHandle) const;
    void generateResumeData() const;
    [[nodiscard]] static std::shared_ptr<const libtorrent::torrent_info> getTorrentFile(const libtorrent::torrent_handle& getHandle);
    [[nodiscard]] static QString getHashString(const libtorrent::torrent_handle& getHandle);
    [[nodiscard]] int64_t bytesToCheck(const libtorrent::torrent_status& status) const;
    void createSession();
    void addFolderGeneric(const QString& key);
    bool addFolderGenericAsync(const QString& key);
    libtorrent::torrent_handle getHandle(const QString& key);
    bool addFolder(const QString& key, const QString& name, bool patchingEnabled);
    void createDeltaManager(const QStringList& deltaUrls);
    libtorrent::torrent_handle getHandleSilent(const QString& key);
    [[nodiscard]] static bool folderChecking(const libtorrent::torrent_status& status);
    [[nodiscard]] static bool folderQueued(const libtorrent::torrent_status& status);
    [[nodiscard]] int downloadEta(const libtorrent::torrent_status& status) const;
    libtorrent::torrent_handle addFolderFromParams(const QString& key);
    static void removeFiles(const QString& hashString);
    void generalInit();
    void generalThreadInit();
    bool removeFolderPriv(const QString& key);
    void setDeltaUrlsPriv(const QStringList &key);
    void setMaxUploadPriv(int limit);
    void setMaxDownloadPriv(int limit);
    void setPortPriv(int port);
    void enableDeltaUpdatesPriv();
    void onFolderAdded(const QString& key, const libtorrent::torrent_handle& handle);
    void shutdown();
};

#endif // LIBTORRENTAPI_H
