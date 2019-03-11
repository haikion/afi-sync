#include <QCoreApplication>
#include <QDirIterator>
#include <QTimer>
#include <QtMath>
#include "afisynclogger.h"
#include "fileutils.h"
#include "global.h"
#include "installer.h"
#include "mod.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

const unsigned Mod::COMPLETION_WAIT_DURATION = 0;

Mod::Mod(const QString& name, const QString& key, ISync* sync):
    SyncItem(name),
    key_(key),
    sync_(sync),
    updateTimer_(nullptr),
    waitTime_(0),
    totalWanted_(-1),
    totalWantedDone_(-1)
{
    LOG << "key = " << key;
    setStatus(SyncStatus::STARTING);
    //Enables non lagging UI
    moveToThread(Global::workerThread);
    qRegisterMetaType<QVector<int>>("QVector<int>"); //TODO: Remove, QML ?
    QMetaObject::invokeMethod(this, &Mod::threadConstructor, Qt::QueuedConnection);
}

Mod::~Mod()
{
    LOG << "name = " << name();
}

void Mod::threadConstructor()
{
    updateTicked();
    updateTimer_ = new QTimer(this);
    updateTimer_->setTimerType(Qt::VeryCoarseTimer);
    updateTimer_->setInterval(1000);
    connect(updateTimer_, &QTimer::timeout, this, &Mod::update);
    connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(repositoryChanged()));
}

QString Mod::path() const
{
    return SettingsModel::modDownloadPath() + "/" + name();
}

void Mod::init()
{
    updateTicked();
    repositoryChanged();
    update();
    LOG << "name = " << name() << " key = " << key() << " Completed";
}

void Mod::update()
{
    updateStatus();
    updateProgress();
}

//Removes mod which has same name but different key.
void Mod::removeConflicting() const
{
    for (const QString& syncKey : sync_->folderKeys())
    {
        QString syncPath = sync_->folderPath(syncKey);

        if (path().toLower() == syncPath.toLower() && syncKey != key_)
        {
            //Downloading into same folder but the key is different!
            LOG << "Removing conflicting (" << syncKey << ", " << syncPath << ") for (" << key_ << ", " << path() << ").";
            sync_->removeFolder(syncKey);
            return;
        }
    }
}

//Starts sync directory. If directory does not exist
//folder will be created. If path is incorrect
//folder will be re-created correctly.
//If mod is not in sync it will be added to it.
void Mod::start()
{
    LOG << "name = " << name();

    if (!sync_->folderExists(key_))
    {
        removeConflicting();
        //Add folder
        sync_->addFolder(key_, name());
    }
    //Sanity checks
    const QString error = sync_->folderError(key_);
    if (!error.isEmpty())
    {
        LOG << "name = " << name() << " Re-adding directory. error = " << error;
        //Disagreement between Sync and AFISync
        sync_->removeFolder(key_);
        sync_->addFolder(key_, name());
        setProcessCompletion(true);
    }
    //Do the actual starting
    sync_->setFolderPaused(key_, false);
    startUpdates();
}

void Mod::moveFiles()
{
    QMetaObject::invokeMethod(this, &Mod::moveFilesSlot, Qt::QueuedConnection);
}

void Mod::moveFilesSlot()
{
    deleteExtraFiles();
    if (sync_->folderExists(key_))
    {
        sync_->setFolderPath(key_, SettingsModel::modDownloadPath());
    }
}

void Mod::setProcessCompletion(const bool value)
{
    SettingsModel::setProcessed(name(), !value ? key_ : "");
    LOG << "Process (completion) set to " << value << " for " << name();
}

bool Mod::getProcessCompletion() const
{
    return SettingsModel::processed(name()) != key_;
}

qint64 Mod::totalWanted() const
{
    return totalWanted_;
}

qint64 Mod::totalWantedDone() const
{
    return totalWantedDone_;
}

void Mod::updateProgress()
{
    if (!active() || !sync_->folderExists(key_))
    {
        totalWanted_ =  -1;
        totalWantedDone_ = -1;
        return;
    }

    totalWanted_ = sync_->folderTotalWanted(key_);
    totalWantedDone_ = sync_->folderTotalWantedDone(key_);
}

bool Mod::stop()
{
    LOG << name();
    //TODO: Remove? Might be too defensive
    if (!sync_->folderExists(key_))
    {
        LOG_ERROR << "Folder" << name() << "does not exist.";
        return false;
    }

    LOG << "Stopping mod transfer. name = " << name();
    sync_->setFolderPaused(key_, true);
    stopUpdatesSlot();
    update();

    return true;
}

void Mod::deleteExtraFiles()
{
    LOG << "name = " << name();
    if (!ticked())
    {
        LOG << "Mod " << name()  << " is inactive, doing nothing.";
        return;
    }

    QSet<QString> localFiles;
    QSet<QString> remoteFiles = sync_->folderFilesUpper(key_);

    QDir dir(SettingsModel::modDownloadPath() + "/" + name());
    QDirIterator it(dir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        localFiles.insert(it.next().toUpper());
    }
    //Fail safe if there is torrent without files.
    if (remoteFiles.size() == 0)
    {
        LOG_ERROR << "Not deleting extra files because torrent contains 0 files.";
        return; //Would delete everything otherwise
    }

    QSet<QString> extraFiles = localFiles - remoteFiles;
    for (const QString& path : extraFiles)
    {
        LOG << "Deleting extra file " << path << " from mod " << name();
        FileUtils::rmCi(path);
    }
    FileUtils::safeRemoveEmptyDirs(dir.absolutePath());
    LOG << "Completed name = " << name();
}

bool Mod::ticked()
{
    return ticked_;
}

void Mod::updateTicked()
{
    if (!optional() && !reposInactive())
    {
        ticked_ = true;
        return;
    }

    for (ModAdapter* adp : modAdapters())
    {
        if (adp->ticked())
        {
            ticked_ = true;
            return;
        }
    }
    ticked_ = false;
}

//If all repositories this mod is included in are disabled then stop the
//download.
void Mod::repositoryChanged()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    if (!sync_->ready())
    {
        return;
    }
    if (reposInactive() || !ticked())
    {
        // No checking for paused state because torrent might have been
        // paused by auto management (queued for example).
        if (sync_->folderExists(key_))
        {
            LOG << "All repositories inactive or mod unchecked. Stopping " << name();
            stop();
        }
        return;
    }
    //At least one repo active and mod checked
    if (!sync_->folderExists(key_) || sync_->folderPaused(key_))
    {
        LOG << "Starting mod transfer. name = " << name();
        start();
    }
    updateTicked();
}

void Mod::removeModAdapter(ModAdapter* modAdapter)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    adapters_.removeAll(modAdapter);
    if (adapters_.isEmpty())
    {
        deleteLater();
    }
}

void Mod::removeModAdapter(Repository* repository)
{
    QList<ModAdapter*> modAdapters = adapters_;
    for (ModAdapter* modAdapter : modAdapters)
    {
        if (modAdapter->repo() == repository)
        {
            delete modAdapter;
        }
    }
}

QSet<Repository*> Mod::repositories() const
{
    QSet<Repository*> retVal;
    for (ModAdapter* modAdapter : adapters_)
    {
        retVal.insert(modAdapter->repo());
    }
    return retVal;
}

QString Mod::key() const
{
    return key_;
}

void Mod::startUpdates()
{
    QMetaObject::invokeMethod(this, &Mod::startUpdatesSlot, Qt::QueuedConnection);
}

void Mod::startUpdatesSlot()
{
    if (sync_->ready())
    {
        updateTimer_->start();
    }
    else
    {
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), updateTimer_, SLOT(start()));
    }
}

//This should always be run in UI Thread
void Mod::stopUpdates()
{
    LOG << name();
    //Updates need to be stopped before object destruction, hence blocking.
    QMetaObject::invokeMethod(this, &Mod::stopUpdatesSlot, Qt::BlockingQueuedConnection);
}

//This should always be run in workerThread
void Mod::stopUpdatesSlot()
{
    updateTimer_->stop();
}

bool Mod::selected()
{
    for (ModAdapter* adp : modAdapters())
    {
        if (adp->selected())
        {
            return true;
        }
    }
    return false;
}

//Returns true only if all adapters are optional
bool Mod::optional()
{
    bool rVal = true;
    for (ModAdapter* adp : modAdapters())
    {
        rVal = rVal && adp->optional();
    }
    return rVal;
}

QList<ModAdapter*> Mod::modAdapters()
{
    adaptersMutex_.lock();
    QList<ModAdapter*> retVal = adapters_;
    adaptersMutex_.unlock();
    return retVal;
}

void Mod::appendModAdapter(ModAdapter* adapter)
{
    adaptersMutex_.lock();
    adapters_.append(adapter);
    int size = adapters_.size();
    adaptersMutex_.unlock();

    if (size == 1)
    {
        //First repository added -> initialize mod.
        if (sync_->ready())
        {
            LOG << "name = " << name() << " Calling init directly";
            QMetaObject::invokeMethod(this, &Mod::init, Qt::QueuedConnection);
        }
        else
        {
            connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(init()));
            LOG << "name = " << name() << " initCompleted connection created";
        }
        return;
    }
}

void Mod::updateStatus()
{
    if (!ticked() || reposInactive())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if (!sync_->folderExists(key_))
    {
        setStatus(SyncStatus::ERRORED);
    }
    else if (sync_->folderMovingFiles(key_))
    {
        setStatus(SyncStatus::MOVING_FILES);
    }
    else if (sync_->folderQueued(key_))
    {
        setStatus(SyncStatus::QUEUED);
    }
    else if (sync_->folderCheckingPatches(key_))
    {
        setStatus(SyncStatus::CHECKING_PATCHES);
    }
    else if (sync_->folderChecking(key_))
    {
        setStatus(SyncStatus::CHECKING);
    }
    //Wait for ready just in case
    else if (statusStr() == SyncStatus::WAITING)
    {
        ++waitTime_;
        if (waitTime_ > COMPLETION_WAIT_DURATION)
        {
            waitTime_ = 0;
            if (getProcessCompletion())
            {
                processCompletion();
                setProcessCompletion(false);
            }
            setStatus(SyncStatus::READY);
        }
    }
    else if (statusStr() == SyncStatus::READY || statusStr() == SyncStatus::READY_PAUSED) {}
    else if (sync_->folderReady(key_))
    {
        setStatus(SyncStatus::WAITING);
    }
    else
    {
        //New block for error initialization
        const QString error = sync_->folderError(key_);
        if (error != QString())
        {
            setStatus(SyncStatus::ERRORED + error);
        }
        else if (sync_->folderPaused(key_))
        {
            if (statusStr() == SyncStatus::READY)
            {
                setStatus(SyncStatus::READY_PAUSED);
            }
            else
            {
                setStatus(SyncStatus::PAUSED);
            }
        }
        else if (sync_->folderExtractingPatch(key_))
        {
            setStatus(SyncStatus::EXTRACTING_PATCH);
        }
        else if (sync_->folderPatching(key_))
        {
            setStatus(SyncStatus::PATCHING);
        }
        else if (sync_->folderNoPeers(key_))
        {
            setStatus(SyncStatus::NO_PEERS);
        }
        else if (sync_->folderDownloadingPatches(key_))
        {
            setStatus(SyncStatus::DOWNLOADING_PATCHES);
        }
        else if (eta() > 0)
        {
            if (statusStr() != SyncStatus::DOWNLOADING)
            {
                setStatus(SyncStatus::DOWNLOADING);
                //Heavy operation -> only set when entering downloading state.
                setProcessCompletion(true);
            }
        }
    }

}

void Mod::forceCheck()
{
    sync_->disableQueue(key_);
    check();
}

QString Mod::progressStr()
{
    if (!ticked() || statusStr() == SyncStatus::STARTING)
        return "???";

    return toProgressStr(totalWanted_, totalWantedDone_);
}

QString Mod::bytesToMegasCeilStr(const qint64 bytes)
{
    if (bytes == 0)
        return QString::number(0);

    return QString::number(1 + ((bytes - 1) / Constants::MEGA_DIVIDER));  // Ceil division
}

QString Mod::toProgressStr(const qint64 totalWanted, qint64 totalWantedDone)
{
    if (totalWanted < 0 || totalWantedDone < 0)
        return "???";

    // Do not ceil to 100 %
    if (totalWantedDone < totalWanted)
    {
        totalWantedDone = totalWantedDone > Constants::MEGA_DIVIDER ? totalWantedDone - Constants::MEGA_DIVIDER : 0;
    }
    return QString("%1 / %2").arg(bytesToMegasCeilStr(totalWantedDone)).arg(bytesToMegasCeilStr(totalWanted));
}

void Mod::processCompletion()
{
    deleteExtraFiles();
    Installer::install(this);
}

void Mod::check()
{
    sync_->checkFolder(key_);
    // Process completion when checking is done
    setProcessCompletion(true);
}

void Mod::checkboxClicked()
{
    QMetaObject::invokeMethod(this, &Mod::checkboxClickedSlot, Qt::QueuedConnection);
}

void Mod::checkboxClickedSlot()
{
    updateTicked();
    //Below cmd will start the download if repository is active.
    repositoryChanged();
}

bool Mod::reposInactive()
{
    for (Repository* repo : repositories())
    {
        if (repo->statusStr() != SyncStatus::INACTIVE)
        {
            return false;
        }
    }
    return true;
}
