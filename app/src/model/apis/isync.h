#ifndef ISYNC_H
#define ISYNC_H

#include <QObject>
#include <QList>
#include <QVariantMap>
#include <QString>

class ISync
{
public:
	ISync() = default;
    virtual ~ISync() = default;

    virtual void mirrorDeltaPatches() = 0;
    virtual QStringList deltaUrls() = 0;
    virtual void disableDeltaUpdates() = 0;
    virtual void enableDeltaUpdates() = 0;

    /**
     * @brief checkFolder
     * Rechecks folder
     * @param key
     */
    virtual void checkFolder(const QString& key) = 0;
    /**
     * @brief folderKeys
     * Returns list of keys of added folders
     * @return
     */
    virtual QStringList folderKeys() = 0;
    //Returns true if there are no peers.
    [[nodiscard]] virtual bool folderNoPeers(const QString& key) = 0;
    //Returns true if folder is moving files
    [[nodiscard]] virtual bool folderMovingFiles(const QString& key) = 0;
    //Returns true if folder with given key has finished (checked & downloaded).
    [[nodiscard]] virtual bool folderReady(const QString& key) = 0;
    //Returns true if folder is queued for checking or downloading
    [[nodiscard]] virtual bool folderQueued(const QString& key) = 0;
    //Returns true if folder is indexing or checking files.
    [[nodiscard]] virtual bool folderChecking(const QString& key) = 0;
    [[nodiscard]] virtual bool folderDownloading(const QString& key) = 0;
    virtual void setDeltaUrls(const QStringList& urls) = 0;
    virtual void setFolderPath(const QString& key, const QString& path, bool overwrite) = 0;
    //Sets folder in paused mode or starts if if value is set to false.
    virtual void setFolderPaused(const QString& key, bool value) = 0;
    [[nodiscard]] virtual bool folderPatching(const QString& key) = 0;
    //Returns true if downloading patches for specific folder.
    [[nodiscard]] virtual bool folderDownloadingPatches(const QString& key) = 0;
    [[nodiscard]] virtual bool folderExtractingPatch(const QString& key) = 0;
    //Removes folder with specific key.
    virtual void removeFolder(const QString& key) = 0;
    //Returns list of files in folder in upper case unix (separator = /) format. Example: "C:/MODS/@MOD/FILE.PBO"
    [[nodiscard]] virtual QSet<QString> folderFilesUpper(const QString& key) = 0;
    //Returns true if folder with specific key exists.
    [[nodiscard]] virtual bool folderExists(const QString& key) = 0;
    //Not supported by libtorrent
    //Returns last modification date. Format: Seconds since epoch
    //virtual int getLastModified(const QString& key) = 0;
    //Returns true if folder is paused
    [[nodiscard]] virtual bool folderPaused(const QString& key) = 0;
    //Returns string describing the error. Returns empty string if no error.
    [[nodiscard]] virtual QString folderError(const QString& key) = 0;
    //Returns file system path of the folder with specific key.
    [[nodiscard]] virtual QString folderPath(const QString& key) = 0;
    //Returns total bandwidths
    [[nodiscard]] virtual int64_t download() const = 0;
    [[nodiscard]] virtual int64_t upload() = 0;
    //Sets global max upload
    virtual void setMaxUpload(int limit) = 0;
    //Sets global max download
    virtual void setMaxDownload(int limit) = 0;
    //Returns true if the sync has loaded and is ready to take commands.
    [[nodiscard]] virtual bool ready() = 0;
    //Sets outgoing port.
    virtual void setPort(int port) = 0;
    //Adds folder, key is source.
    [[nodiscard]] virtual bool addFolder(const QString& key, const QString& name) = 0;
    virtual void disableQueue(const QString& key) = 0;
    [[nodiscard]] virtual int64_t folderFileSize(const QString& key) = 0;
    [[nodiscard]] virtual int64_t folderTotalWanted(const QString& key) = 0;
    [[nodiscard]] virtual int64_t folderTotalWantedDone(const QString& key) = 0;
    virtual void cleanUnusedFiles(const QSet<QString>& usedKeys) = 0;
    [[nodiscard]] virtual bool folderCheckingPatches(const QString& key) = 0;

signals: // <- ignored by moc and only serves as documentation aid
    // The code will work exactly the same if signals: is absent.
    void initCompleted();

    Q_DISABLE_COPY(ISync)
};

Q_DECLARE_INTERFACE(ISync, "ISync") // define this out of namespace scope

#endif // ISYNC_H
