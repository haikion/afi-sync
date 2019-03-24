#ifndef ISYNC_H
#define ISYNC_H

#include <QObject>
#include <QList>
#include <QVariantMap>
#include <QString>

class ISync
{
public:
    virtual ~ISync() = default;

    virtual void setDeltaUpdatesFolder(const QString& key) = 0;
    virtual QString deltaUpdatesKey() = 0;
    virtual bool disableDeltaUpdates() = 0;
    virtual void enableDeltaUpdates() = 0;

    //Rechecks folder
    virtual void checkFolder(const QString& key) = 0;
    //Returns list of keys of added folders.
    virtual QList<QString> folderKeys() = 0;
    //Returns true if there are no peers.
    virtual bool folderNoPeers(const QString& key) = 0;
    //Returns true if folder is moving files
    virtual bool folderMovingFiles(const QString& key) = 0;
    //Returns true if folder with given key has finished (checked & downloaded).
    virtual bool folderReady(const QString& key) = 0;
    //Returns true if folder is queued for checking or downloading
    virtual bool folderQueued(const QString& key) = 0;
    //Returns true if folder is indexing or checking files.
    virtual bool folderChecking(const QString& key) = 0;
    virtual void setFolderPath(const QString& key, const QString& path) = 0;
    //Sets folder in paused mode or starts if if value is set to false.
    virtual void setFolderPaused(const QString& key, bool value) = 0;
    //Fetches eta to ready state. Returns time in seconds.
    virtual int folderEta(const QString& key) = 0;
    virtual bool folderPatching(const QString& key) = 0;
    //Returns true if downloading patches for specific folder.
    virtual bool folderDownloadingPatches(const QString& key) = 0;
    virtual bool folderExtractingPatch(const QString& key) = 0;
    //Removes folder with specific key.
    virtual void removeFolder(const QString& key) = 0;
    //Returns list of files in folder in upper case unix (separator = /) format. Example: "C:/MODS/@MOD/FILE.PBO"
    virtual QSet<QString> folderFilesUpper(const QString& key) = 0;
    //Returns true if folder with specific key exists.
    virtual bool folderExists(const QString& key) = 0;
    //Not supported by libtorrent
    //Returns last modification date. Format: Seconds since epoch
    //virtual int getLastModified(const QString& key) = 0;
    //Returns true if folder is paused
    virtual bool folderPaused(const QString& key) = 0;
    //Returns string describing the error. Returns empty string if no error.
    virtual QString folderError(const QString& key) = 0;
    //Returns file system path of the folder with specific key.
    virtual QString folderPath(const QString& key) = 0;
    //Returns total bandwidths
    virtual qint64 download() const = 0;
    virtual qint64 upload() = 0;
    //Sets global max upload
    virtual void setMaxUpload(const int limit) = 0;
    //Sets global max download
    virtual void setMaxDownload(const int limit) = 0;
    //Returns true if the sync has loaded and is ready to take commands.
    virtual bool ready() = 0;
    //Sets outgoing port.
    virtual void setPort(int port) = 0;
    //Adds folder, key is source.
    virtual bool addFolder(const QString& key, const QString& name) = 0;
    virtual void disableQueue(const QString& key) = 0;
    virtual qint64 folderTotalWanted(const QString& key) = 0;
    virtual qint64 folderTotalWantedDone(const QString& key) = 0;
    virtual void cleanUnusedFiles(const QSet<QString> usedKeys) = 0;
    virtual bool folderCheckingPatches(const QString& key) = 0;

signals: // <- ignored by moc and only serves as documentation aid
    // The code will work exactly the same if signals: is absent.
    void initCompleted();
};

Q_DECLARE_INTERFACE(ISync, "ISync") // define this out of namespace scope

#endif // ISYNC_H
