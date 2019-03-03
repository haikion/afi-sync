#ifndef LIBTORRENTAPI_H
#define LIBTORRENTAPI_H

#include <vector>

#include <QObject>
#include <QHash>
#include <QDir>
#include <QTimer>
#include <QSet>

#pragma warning(push, 0)
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/alert.hpp>
#pragma warning(pop)

#include "../../cihash.h"
#include "../isync.h"
#include "alerthandler.h"
#include "deltamanager.h"
#include "speedestimator.h" // TODO: Remove?
#include "storagemovemanager.h"

// TODO: Split this into more classes in order to reduce complexity.
// create class for managing settings
class LibTorrentApi : public QObject, public ISync
{
    Q_OBJECT
    Q_INTERFACES(ISync)

public:
    explicit LibTorrentApi();
    explicit LibTorrentApi(const QString& deltaUpdatesKey);
    ~LibTorrentApi();

    void setDeltaUpdatesFolder(const QString& key) override;
    QString deltaUpdatesKey() override;
    bool disableDeltaUpdates() override;
    bool disableDeltaUpdatesNoTorrents();
    void enableDeltaUpdates() override;

    void checkFolder(const QString& key) override;
    //Returns true if there are no peers.
    bool folderNoPeers(const QString& key) override;
    //Returns list of keys of added folders.
    QList<QString> folderKeys() override;
    bool folderReady(const QString& key) override;
    //Returns true if folder is indexing or checking files.
    bool folderChecking(const QString& key) override;
    bool folderMovingFiles(const QString& key) override;
    bool folderQueued(const QString& key) override;
    //Fetches eta to ready state. Returns time in seconds.
    int folderEta(const QString& key) override;
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
    qint64 download() const override;
    qint64 upload() override;
    //Sets global max upload
    void setMaxUpload(const int limit) override;
    //Sets global max download
    void setMaxDownload(const int limit) override;
    //Returns true if the sync has loaded and is ready to take commands.
    bool ready() override;
    //Sets outgoing port.
    void setPort(int port) override;
    //Restarts sync
    void start() override;
    bool folderDownloadingPatches(const QString& key) override;
    void disableQueue(const QString& key) override;
    qint64 folderTotalWanted(const QString& key) override;
    qint64 folderTotalWantedDone(const QString& key) override;
    void cleanUnusedFiles(const QSet<QString> usedKeys) override;
    bool folderExtractingPatch(const QString& key) override;
    bool folderCheckingPatches(const QString& key) override;

private slots:
    void handleAlerts();
    void handlePatched(const QString& key, const QString& modName, bool success);
    void init();
    bool removeFolderSlot(const QString& key);    
    void setMaxUploadSlot(const int limit);
    void setMaxDownloadSlot(const int limit);
    void setPortSlot(int port);
    void enableDeltaUpdatesSlot();
    void initDelta();
    void shutdown();    

signals:
    void initCompleted();

private:
    static const int NOT_FOUND; //Unable to fetch eta
    static const QString ERROR_KEY_NOT_FOUND;
    static const QString ERROR_SESSION_NULL;

    QTimer* timer_;
    libtorrent::session* session_;
    // TODO: Create single type called TorrentHandle
    // and combine these.
    CiHash<libtorrent::torrent_handle> keyHash_;
    CiHash<libtorrent::add_torrent_params> torrentParams_;
    CiHash<QString> prefixMap_;

    DeltaManager* deltaManager_;
    int numResumeData_;
    SpeedEstimator speedEstimator_;
    QString deltaUpdatesKey_;
    QString settingsPath_;
    int64_t checkingSpeed_; // TODO: Remove?
    AlertHandler* alertHandler_;
    StorageMoveManager* storageMoveManager_;
    std::atomic<bool> ready_;

    bool loadLtSettings();
    void saveSettings();
    boost::shared_ptr<libtorrent::torrent_info> loadFromFile(const QString& path) const;
    void loadTorrentFiles(const QDir& dir);
    bool saveTorrentFile(const libtorrent::torrent_handle& getHandle) const;
    void generateResumeData() const;
    std::vector<char> loadResumeData(const QString& path) const;
    boost::shared_ptr<const libtorrent::torrent_info> getTorrentFile(const libtorrent::torrent_handle& getHandle) const;
    QString getHashString(const libtorrent::torrent_handle& getHandle) const;
    int64_t bytesToCheck(const libtorrent::torrent_status& status) const;
    bool createSession();
    libtorrent::torrent_handle addFolderGeneric(const QString& key);
    libtorrent::torrent_handle addFolderGenericAsync(const QString& key);
    int folderEta(libtorrent::torrent_handle getHandle);
    libtorrent::torrent_handle getHandle(const QString& key);
    bool addFolder(const QString& key, const QString& name, bool patchingEnabled);
    void createDeltaManager(libtorrent::torrent_handle handle, const QString& key);
    libtorrent::torrent_handle getHandleSilent(const QString& key);
    bool folderChecking(const libtorrent::torrent_status& status) const;
    bool folderQueued(const libtorrent::torrent_status& status) const;
    int queuedCheckingEta(const libtorrent::torrent_status& status) const;
    int queuedDownloadEta(const libtorrent::torrent_status& status) const;
    int downloadEta(const libtorrent::torrent_status& status) const;
    libtorrent::torrent_handle addFolderFromParams(const QString& key);
    void removeFiles(const QString& hashString);
    void generalThreadInit();
    void generalInit();
    qint64 folderTotalWantedMoving(const QString& key);
};

#endif // LIBTORRENTAPI_H
