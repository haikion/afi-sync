#include "deltamanager.h"

#include <chrono>

#include <QDirIterator>
#include <QFileInfo>
#include <QThread>

#include "model/afisync.h"
#include "model/afisynclogger.h"
#include "model/fileutils.h"
#include "model/global.h"
#include "model/settingsmodel.h"

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace lt = libtorrent;

DeltaManager::DeltaManager(const QStringList& deltaUrls, lt::session* session, QObject* parent):
    QObject(parent)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    patcher_ = new DeltaPatcher(SettingsModel::modDownloadPath() + u"/afisync_patches"_s);
    patcher_->moveToThread(&patcherThread_);
    connect(patcher_, &DeltaPatcher::patched, this, &DeltaManager::handlePatched);
    patcherThread_.setObjectName("Patcher Thread");
    patcherThread_.start();

    updateTimer_.setInterval(1s);
    connect(&updateTimer_, &QTimer::timeout, this, &DeltaManager::update);
    removeDeprecatedPatches(deltaUrls);
    downloader_.setSession(session);
    downloader_.setDeltaUrls(deltaUrls);
}

DeltaManager::~DeltaManager()
{
    delete patcher_;
    patcherThread_.quit();
    patcherThread_.wait(10000);
}

void DeltaManager::mirrorDeltaPatches()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    downloader_.mirrorDeltaPatches();
}

bool DeltaManager::patchAvailable(const QString& modName)
{
    return downloader_.patchAvailable(modName);
}

void DeltaManager::update()
{
    if (patcher_->isPatching())
    {
        return;
    }

    QString downloadedPatchMod = findDownloadedMod();
    if (!downloadedPatchMod.isEmpty())
    {
        patcher_->patch(SettingsModel::modDownloadPath() + "/" + downloadedPatchMod);
        inDownload_.remove(keyHash_.key(downloadedPatchMod));
    }
}

QString DeltaManager::findDownloadedMod() const
{
    for (const QString& key: std::as_const(inDownload_))
    {
        if (downloader_.patchesDownloaded(key))
        {
            return keyHash_.value(key);
        }
    }
    return {};
}

void DeltaManager::removeDeprecatedPatches(const QStringList& urls)
{
    const QString patchesPath = SettingsModel::modDownloadPath()  + u"/afisync_patches"_s;
    QDir dir(patchesPath);
    QStringList files = dir.entryList(QDir::Files);
    QStringList deprecatedPatches = AfiSync::filterDeprecatedPatches(files, urls);
    for (const auto& file : deprecatedPatches)
    {
        FileUtils::rmCi(patchesPath + '/' + file);
    }
}

CiHash<QString> DeltaManager::keyHash() const
{
    return keyHash_;
}

void DeltaManager::patch(const QString& modName, const QString& key)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    LOG << "Patching " << modName << " ...";
    keyHash_.insert(key, modName);
    updateTimer_.start(); // Start update timer if it isn't already running
    inDownload_.insert(key);
    downloader_.downloadPatches(modName, key);
    //Patch will be recalled once download is completed.
}

void DeltaManager::setDeltaUrls(const QStringList& deltaUrls)
{
    removeDeprecatedPatches(deltaUrls);
    downloader_.setDeltaUrls(deltaUrls);
}

QStringList DeltaManager::deltaUrls() const
{
    return downloader_.deltaUrls();
}

bool DeltaManager::contains(const QString& key)
{
    return keyHash_.contains(key) || inDownload_.contains(key);
}

QString DeltaManager::name(const QString& key)
{
    return keyHash_.value(key, {});
}

bool DeltaManager::patchDownloading(const QString& key) const
{
    return inDownload_.contains(key);
}

libtorrent::torrent_handle DeltaManager::getHandle(const QString& key) const
{
    return downloader_.getHandle(key);
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
    if (downloader_.patchesDownloaded(modName))
    {
        return patcher_->totalBytes(modName);
    }
    return downloader_.totalWanted(modName);
}

int64_t DeltaManager::totalWantedDone(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it == keyHash_.end())
    {
        LOG_ERROR << "Key " << key << " not found!";
        return -1;
    }
    const QString modName = it.value();
    if (downloader_.patchesDownloaded(modName))
    {
        return patcher_->bytesPatched(modName);
    }
    return downloader_.totalWantedDone(modName);
}

QStringList DeltaManager::folderKeys()
{
    return keyHash_.keys();
}

void DeltaManager::handlePatched(const QString& modPath, bool success)
{
    const QString modName = QFileInfo(modPath).fileName();
    const QString key = keyHash_.key(modName);
    LOG << modName << " patched";
    keyHash_.remove(key);
    if (keyHash_.empty())
    {
        updateTimer_.stop();
    }

    emit patched(key, modName, success);
}

bool DeltaManager::queued(const QString& key)
{
    Q_ASSERT(!key.isEmpty());
    auto it = keyHash_.find(key);
    if (it == keyHash_.end())
    {
        return false;
    }

    const QString modName = it.value();
    return downloader_.patchesDownloaded(modName) && !patcher_->patching(modName);
}

const QList<libtorrent::torrent_handle> DeltaManager::handles() const
{
    return downloader_.handles();
}

QString DeltaManager::getUrl(const libtorrent::torrent_handle& handle) const
{
    return downloader_.getUrl(handle);
}

bool DeltaManager::patchExtracting(const QString& key)
{
    const QString modName = keyHash_.value(key, {});
    if (modName.isEmpty())
    {
        return false;
    }

    return patcher_->patching(modName) && patcher_->patchExtracting();
}

bool DeltaManager::patching(const QString& key)
{
    const QString modName = keyHash_.value(key, {});
    if (modName.isEmpty())
    {
        return false;
    }

    return patcher_->patching(modName);
}
