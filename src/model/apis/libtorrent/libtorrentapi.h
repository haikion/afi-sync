#ifndef LIBTORRENTAPI_H
#define LIBTORRENTAPI_H

#include <vector>

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/alert.hpp>

#include <QObject>
#include <QHash>
#include <QDir>
#include <QTimer>
#include <QSet>
#include "../../cihash.h"
#include "../isync.h"
#include "speedestimator.h"
#include "deltamanager.h"

class LibTorrentApi : public QObject, public ISync
{
    Q_OBJECT
    Q_INTERFACES(ISync)

public:
    explicit LibTorrentApi(QObject *parent = 0);
    ~LibTorrentApi();

    virtual void setDeltaUpdatesFolder(const QString& key);
    virtual QString deltaUpdatesKey();
    virtual bool disableDeltaUpdates();
    bool disableDeltaUpdatesNoTorrents();
    virtual bool enableDeltaUpdates();

    virtual void checkFolder(const QString& key);
    //Returns true if there are no peers.
    virtual bool folderNoPeers(const QString& key);
    //Returns list of keys of added folders.
    virtual QList<QString> folderKeys();
    virtual bool folderReady(const QString& key);
    //Returns true if folder is indexing or checking files.
    virtual bool folderChecking(const QString& key);
    virtual bool folderQueued(const QString& key);
    //Fetches eta to ready state. Returns time in seconds.
    virtual int folderEta(const QString& key);
    virtual bool folderPatching(const QString& key);
    //Adds folder, path is local system directory, key is source.
    virtual bool addFolder(const QString& key, const QString& name);
    //Removes folder with specific key.
    virtual bool removeFolder(const QString& key);
    //Returns list of files in folder in upper case format.
    virtual QSet<QString> folderFilesUpper(const QString& key);
    //Returns true if folder with specific key exists.
    virtual bool folderExists(const QString& key);
    void setFolderPath(const QString& key, const QString& path);
    //Returns true if folder is paused
    virtual bool folderPaused(const QString& key);
    //Sets folder in paused mode or starts if if value is set to false.
    virtual void setFolderPaused(const QString& key, bool value);
    //Returns string describing the error. Returns empty string if no error.
    virtual QString folderError(const QString& key);
    //Returns file system path of the folder with specific key.
    virtual QString folderPath(const QString& key);
    //Shutdowns the sync
    virtual void shutdown();
    //Total bandwiths
    virtual qint64 download() const;
    virtual qint64 upload();
    //Sets global max upload
    virtual void setMaxUpload(const unsigned limit);
    //Sets global max download
    virtual void setMaxDownload(const unsigned limit);
    //Returns true if the sync has loaded and is ready to take commands.
    virtual bool ready();
    //Sets outgoing port.
    virtual void setPort(int port);
    //Restarts sync
    virtual void start();    
    virtual bool folderDownloadingPatches(const QString& key);
    virtual void disableQueue(const QString& key);

private slots:
    void handleAlerts();
    void handlePatched(const QString& key, const QString& modName, bool success);

signals:
    void initCompleted();

private:
    static const int NOT_FOUND; //Unable to fetch eta
    static const QString ERROR_KEY_NOT_FOUND;
    static const QString ERROR_SESSION_NULL;

    QTimer alertTimer_;
    libtorrent::session* session_;
    CiHash<libtorrent::torrent_handle> keyHash_;
    DeltaManager* deltaManager_;
    int numResumeData_;
    std::vector<libtorrent::alert*>* alerts_;
    SpeedEstimator speedEstimator_;
    QString deltaUpdatesKey_;
    QString settingsPath_;
    int64_t checkingSpeed_;

    void init();
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
    void handleAlert(libtorrent::alert* a);
    void handleListenFailedAlert(const libtorrent::listen_failed_alert* a) const;
    void handleTorrentCheckAlert(const libtorrent::torrent_checked_alert* a) const;
    void handleTorrentFinishedAlert(const libtorrent::torrent_finished_alert* a);
    void handleListenSucceededAlert(const libtorrent::listen_succeeded_alert* a) const;
    void handleFastresumeRejectedAlert(const libtorrent::fastresume_rejected_alert* a) const;
    void handleMetadataReceivedAlert(const libtorrent::metadata_received_alert* a) const;
    void handleMetadataFailedAlert(const libtorrent::metadata_failed_alert* a) const;
    void handlePortmapErrorAlert(const libtorrent::portmap_error_alert* a) const;
    void handlePortmapAlert(const libtorrent::portmap_alert* a) const;
    libtorrent::torrent_handle getHandleSilent(const QString& key);
    bool folderChecking(const libtorrent::torrent_status& status) const;
    bool folderQueued(const libtorrent::torrent_status& status) const;
    int queuedCheckingEta(const libtorrent::torrent_status& status) const;
    int queuedDownloadEta(const libtorrent::torrent_status& status) const;
    int downloadEta(const libtorrent::torrent_status& status) const;
    int checkingEta(const libtorrent::torrent_status& status);
};

#endif // LIBTORRENTAPI_H
