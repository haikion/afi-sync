#ifndef LIBTORRENTAPI_H
#define LIBTORRENTAPI_H

#include <QObject>
#include <QDir>
#include <QHash>
#include <QNetworkAccessManager>
#include <QSet>
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

class LibTorrentApi : public QObject, virtual public ISync
{
    Q_OBJECT
    Q_INTERFACES(ISync)

public:
    explicit LibTorrentApi();
    explicit LibTorrentApi(const QString& deltaUpdatesKey);
    ~LibTorrentApi() override;

    void setDeltaUpdatesFolder(const QString& key) override;
    QString deltaUpdatesKey() override;
    bool disableDeltaUpdates() override;
    bool disableDeltaUpdatesNoTorrents();
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
    int64_t download() const override;
    int64_t upload() override;
    //Sets global max upload
    void setMaxUpload(const int limit) override;
    //Sets global max download
    void setMaxDownload(const int limit) override;
    //Returns true if the sync has loaded and is ready to take commands.
    bool ready() override;
    //Sets outgoing port.
    void setPort(int port) override;
    bool folderDownloadingPatches(const QString& key) override;
    void disableQueue(const QString& key) override;
    qint64 folderTotalWanted(const QString& key) override;
    qint64 folderTotalWantedDone(const QString& key) override;
    void cleanUnusedFiles(const QSet<QString>& usedKeys) override;
    bool folderExtractingPatch(const QString& key) override;
    bool folderCheckingPatches(const QString& key) override;

private slots:
    void handleAlerts();
    void handlePatched(const QString& key, const QString& modName, bool success);
    void init();
    bool removeFolderSlot(const QString& key);
    void setDeltaUpdatesFolderSlot(const QString& key);
    void setMaxUploadSlot(const int limit);
    void setMaxDownloadSlot(const int limit);
    void setPortSlot(int port);
    void enableDeltaUpdatesSlot();
    void initDelta();
    void shutdown();    

signals:
    void initCompleted();
    void shutdownCompleted();
    void folderAdded(QString key);

private:
    static const int NOT_FOUND;
    static const QString ERROR_KEY_NOT_FOUND;
    static const QString ERROR_SESSION_NULL;

    QTimer* timer_;
    QNetworkAccessManager* networkAccessManager_{nullptr};
    libtorrent::session* session_;
    // TODO: Create single type called TorrentHandle
    // and combine these.
    CiHash<libtorrent::torrent_handle> keyHash_;
    CiHash<libtorrent::add_torrent_params> torrentParams_;
    CiHash<QString> prefixMap_;

    AlertHandler* alertHandler_;
    DeltaManager* deltaManager_;
    QQueue<QPair<QString, QString>> pendingFolder_;
    QString deltaUpdatesKey_;
    QString settingsPath_;
    StorageMoveManager* storageMoveManager_;
    int numResumeData_;
    libtorrent::time_point statsLastTimestamp_;
    int64_t downloadSpeed_{0};
    int64_t uploadSpeed_{0};
    std::atomic<bool> ready_;

    std::shared_ptr<libtorrent::torrent_info> loadFromFile(const QString& path) const;
    void loadTorrentFiles(const QDir& dir);
    bool saveTorrentFile(const libtorrent::torrent_handle& getHandle) const;
    void generateResumeData() const;
    std::shared_ptr<const libtorrent::torrent_info> getTorrentFile(const libtorrent::torrent_handle& getHandle) const;
    QString getHashString(const libtorrent::torrent_handle& getHandle) const;
    int64_t bytesToCheck(const libtorrent::torrent_status& status) const;
    void createSession();
    void addFolderGeneric(const QString& key);
    bool addFolderGenericAsync(const QString& key);
    libtorrent::torrent_handle getHandle(const QString& key);
    bool addFolder(const QString& key, const QString& name, bool patchingEnabled);
    void createDeltaManager(libtorrent::torrent_handle handle, const QString& key);
    libtorrent::torrent_handle getHandleSilent(const QString& key);
    bool folderChecking(const libtorrent::torrent_status& status) const;
    bool folderQueued(const libtorrent::torrent_status& status) const;
    int downloadEta(const libtorrent::torrent_status& status) const;
    libtorrent::torrent_handle addFolderFromParams(const QString& key);
    void removeFiles(const QString& hashString);
    void generalThreadInit();
    void generalInit();
    qint64 folderTotalWantedMoving(const QString& key);
    void onFolderAdded(const QString &key, libtorrent::torrent_handle handle);
};

#endif // LIBTORRENTAPI_H
