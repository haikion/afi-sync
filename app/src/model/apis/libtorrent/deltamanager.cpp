#include <QFileInfo>
#include <QDirIterator>
#include <QThread>
#include "libtorrent/torrent_info.hpp"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "../../fileutils.h"
#include "deltamanager.h"

namespace lt = libtorrent;

DeltaManager::DeltaManager(lt::torrent_handle handle, QObject* parent):
    QObject(parent),
    downloader_(new DeltaDownloader(handle)),
    patcher_(new DeltaPatcher(SettingsModel::modDownloadPath()
                              + "/" + Constants::DELTA_PATCHES_NAME, handle)),
    handle_(handle)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    connect(patcher_, &DeltaPatcher::patched, this, &DeltaManager::handlePatched);
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
        if (patcher_->notPatching() && downloader_->patchDownloaded(modName) && !checkingFiles())
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
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (!patchAvailable(modName))
    {
        LOG_WARNING << "No patches found for" << modName;
        return false;
    }

    keyHash_.insert(key, modName);
    updateTimer_.start(); // Start update timer if it isn't already running
    inDownload_.insert(modName);
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
        LOG_ERROR << "Key " << key << " not found.";
        return -1;
    }
    const QString modName = it.value();
    if (downloader_->patchDownloaded(modName))
    {
        return patcher_->totalBytes(modName);
    }
    return downloader_->totalWanted(modName);
}

int64_t DeltaManager::totalWantedDone(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it == keyHash_.end())
    {
        LOG_ERROR << "Key" << key << "not found.";
        return -1;
    }
    const QString modName = it.value();
    if (downloader_->patchDownloaded(modName))
    {
        return patcher_->bytesPatched(modName);
    }
    return downloader_->totalWantedDone(modName);
}

// TODO: Remove, ETA
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
    const QString modName = QFileInfo(modPath).fileName();
    const QString key = keyHash_.key(modName);
    LOG << key << " " << modName;
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

    QSet<QString> extraFiles = localFiles - torrentFiles;
    for (const QString& filePath : extraFiles)
    {
        LOG << "Deleting " << filePath;
        FileUtils::rmCi(filePath);
    }
}

bool DeltaManager::checkingFiles()
{
    const auto status = handle_.status().state;
    return status == libtorrent::torrent_status::state_t::checking_files
            || status == libtorrent::torrent_status::state_t::queued_for_checking
            || status == libtorrent::torrent_status::state_t::checking_resume_data;
}

bool DeltaManager::queued(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it == keyHash_.end())
        return false;

    const QString modName = it.value();
    return downloader_->patchDownloaded(modName) && !patcher_->patching(modName) && !checkingFiles();
}

bool DeltaManager::patchExtracting(const QString& key)
{
    const QString modName = keyHash_.value(key, "");
    if (modName.isEmpty())
        return false;

    return patcher_->patching(modName) && patcher_->patchExtracting();
}

bool DeltaManager::patching(const QString& key)
{
    const QString modName = keyHash_.value(key, "");
    if (modName.isEmpty())
        return false;

    return patcher_->patching(modName);
}

QSet<QString> DeltaManager::torrentFilesUpper()
{
    QSet<QString> rVal;
    lt::torrent_handle handle = downloader_->handle();
    if (!handle.is_valid())
        return rVal;

    auto torrentFile = handle.torrent_file();
    if (!torrentFile)
    {
        LOG_ERROR << "torrent_file is null";
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
