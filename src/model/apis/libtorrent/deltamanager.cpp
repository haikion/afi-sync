#include <QFileInfo>
#include <QDirIterator>
#include <QThread>
#include "libtorrent/torrent_info.hpp"
#include "../../debug.h"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "../../fileutils.h"
#include "deltamanager.h"

namespace lt = libtorrent;

DeltaManager::DeltaManager(lt::torrent_handle handle, QObject* parent):
    QObject(parent),
    downloader_(new DeltaDownloader(handle)),
    patcher_(new DeltaPatcher(SettingsModel::modDownloadPath()
                              + "/" + Constants::DELTA_PATCHES_NAME)),
    handle_(handle)
{
    connect(patcher_, SIGNAL(patched(QString, bool)), this, SLOT(handlePatched(QString, bool)));
    updateTimer_.setInterval(1000);
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
}

DeltaManager::~DeltaManager()
{
    delete patcher_;
    delete downloader_;
}

bool DeltaManager::patchAvailable(const QString& modName)
{
    return downloader_->patchAvailable(modName);
}

void DeltaManager::update()
{
    QSet<QString> inDownload = inDownload_; //Prevents crash because loop edits itself.
    for (const QString& modName : inDownload)
    {
        if (patcher_->notPatching() && downloader_->patchDownloaded(modName))
        {
            patcher_->patch(SettingsModel::modDownloadPath() + "/" + modName);
            inDownload_.remove(modName);
        }
    }
}

CiHash<QString> DeltaManager::keyHash() const
{
    return keyHash_;
}

bool DeltaManager::patch(const QString& modName, const QString& key)
{
    if (!patchAvailable(modName))
        return false;

    keyHash_.insert(key, modName);
    inDownload_.insert(modName);
    DBG << "Starting updates";
    QMetaObject::invokeMethod(&updateTimer_, "start", Qt::QueuedConnection);
    if (downloader_->patchDownloaded(modName))
    {
        patcher_->patch(SettingsModel::modDownloadPath() + "/" + modName);
        return true;
    }
    downloader_->downloadPatch(modName);
    return true;
    //Patch will be recalled once download is completed.
}

bool DeltaManager::contains(const QString& key)
{
    return keyHash_.contains(key);
}

QString DeltaManager::name(const QString& key)
{
    return keyHash_.value(key, "");
}

bool DeltaManager::patchDownloading(const QString& key) const
{
    QString modName = keyHash_.value(key, "");
    if (modName.isEmpty())
        return false;

    boost::int64_t totalWanted = downloader_->totalWanted(modName);
    return totalWanted > 0 && downloader_->totalWantedDone(modName) < totalWanted;
}

libtorrent::torrent_handle DeltaManager::handle() const
{
    return handle_;
}

int64_t DeltaManager::totalWanted(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it == keyHash_.end())
    {
        DBG << "ERROR: Key" << key << "not found.";
        return 99999999;
    }

    return downloader_->totalWanted(it.value());
}

int64_t DeltaManager::totalWantedDone(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it == keyHash_.end())
    {
        DBG << "ERROR: Key" << key << "not found.";
        return 99999999;
    }

    return downloader_->totalWantedDone(it.value());
}

int DeltaManager::patchingEta(const QString& key)
{
    static const qint64 SPEED =  600000;
    QString modName = keyHash_.value(key);

    //Returns extracted if being patched (more accurate)
    qint64 totalBytes = patcher_->totalBytes(modName);
    if (totalBytes == 0) //If downloading, return 7z size
        totalBytes = totalWanted(key) * 1.4;

    qint64 bytesPatched = patcher_->bytesPatched(modName);
    qint64 bytesReq = totalBytes - bytesPatched;
    return bytesReq/SPEED;
}

QStringList DeltaManager::folderKeys()
{
    return keyHash_.keys();
}

void DeltaManager::handlePatched(const QString& modPath, bool success)
{
    QString modName = QFileInfo(modPath).fileName();
    QString key = keyHash_.key(modName);
    DBG << key << modName;
    keyHash_.remove(key);
    if (keyHash_.empty())
    {
        deleteExtraFiles(); //Cleanup residue from older delta patches torrent.
        updateTimer_.stop();
    }

    emit patched(key, modName, success);
}

void DeltaManager::deleteExtraFiles()
{
    QString savePath = QString::fromStdString(downloader_->handle().status().save_path);
    QSet<QString> torrentFiles = torrentFilesUpper();
    QSet<QString> localFiles;
    QDirIterator it(savePath + "/" + Constants::DELTA_PATCHES_NAME);
    while (it.hasNext())
    {
        QFileInfo fi = it.next();
        if (fi.isFile())
            localFiles.insert(fi.absoluteFilePath().toUpper());
    }
    DBG << savePath << localFiles << torrentFiles;

    QSet<QString> extraFiles = localFiles - torrentFiles;
    for (const QString& filePath : extraFiles)
    {
        DBG << "Deleting" << filePath;
        FileUtils::rmCi(filePath);
    }
}

QSet<QString> DeltaManager::torrentFilesUpper()
{
    QSet<QString> rVal;
    lt::torrent_handle handle = downloader_->handle();
    if (!handle.is_valid())
        return rVal;

    boost::shared_ptr<const lt::torrent_info> torrentFile = handle.torrent_file();
    if (!torrentFile)
    {
        DBG << "ERROR: torrent_file is null";
        return rVal;
    }
    lt::file_storage files = torrentFile->files();
    for (int i = 0; i < files.num_files(); ++i)
    {
        lt::torrent_status status = handle.status(lt::torrent_handle::query_save_path);
        QString value = QString::fromStdString(status.save_path + "/" + files.file_path(i));
        value = QFileInfo(value).absoluteFilePath().toUpper();
        rVal.insert(value);
    }
    return rVal;

}
