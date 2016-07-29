#include <QFileInfo>
#include <QDirIterator>
#include "libtorrent/torrent_info.hpp"
#include "../../debug.h"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "../../fileutils.h"
#include "deltamanager.h"

namespace lt = libtorrent;

DeltaManager::DeltaManager(lt::torrent_handle handle, QObject* parent):
    QObject(parent),
    downloader_(new DeltaDownloader(handle, this)),
    patcher_(new DeltaPatcher(SettingsModel::modDownloadPath()
                              + "/" + Constants::DELTA_PATCHES_NAME)),
    handle_(handle),
    updateTimer_(nullptr)
{
    connect(patcher_, SIGNAL(patched(QString, bool)), this, SLOT(handlePatched(QString, bool)));
}

DeltaManager::~DeltaManager()
{
    delete patcher_; //Cannot be automanaged due to threads
}

bool DeltaManager::patchAvailable(const QString& modName)
{
    return downloader_->patchAvailable(modName);
}

void DeltaManager::update()
{
    for (QString modName : keyHash_.values())
    {
        if (downloader_->patchDownloaded(modName))
            patcher_->patch(SettingsModel::modDownloadPath() + "/" + modName);
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
    if (!updateTimer_)
    {
        DBG << "Starting updates";
        updateTimer_ = new QTimer(this);
        updateTimer_->setInterval(1000);
        connect(updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
        updateTimer_->start();
    }
    if (downloader_->patchDownloaded(modName))
    {
        patcher_->patch(SettingsModel::modDownloadPath() + "/" + modName);
        return true;
    }
    downloader_->downloadPatch(modName);
    DBG << handle_.status().save_path.c_str();
    return true;
    //Patch will be recalled once download is completed.
}

bool DeltaManager::contains(const QString& key)
{
    return keyHash_.contains(key);
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
        //Stop updates
        if (updateTimer_)
        {
            delete updateTimer_;
            updateTimer_ = nullptr;
        }
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
