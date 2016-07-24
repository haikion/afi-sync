#include <QFileInfo>
#include "../../debug.h"
#include "../../global.h"
#include "../../settingsmodel.h"
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
    DBG << "Updating...";
    for (QString modName : keyHash_.values())
    {
        if (downloader_->patchDownloaded(modName))
            patcher_->patch(SettingsModel::modDownloadPath() + "/" + modName);
    }
    //Stop updates if there is nothing to process.
    if (keyHash_.empty())
    {
        DBG << "Nothing to process, stopping updates.";
        delete updateTimer_;
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
    emit patched(key, modName, success);
}
