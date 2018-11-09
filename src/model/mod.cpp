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
    SyncItem(name, 0),
    key_(key),
    sync_(sync),
    updateTimer_(nullptr),
    waitTime_(0)
{
    LOG << "key = " << key;
    setStatus(SyncStatus::NO_SYNC_CONNECTION);
    //Enables non lagging UI
    moveToThread(Global::workerThread);
    qRegisterMetaType<QVector<int>>("QVector<int>");
    //Blocking is required, because otherwise startUpdates might be called before updateTimer_ is initialized.
    QMetaObject::invokeMethod(this, "threadConstructor", Qt::BlockingQueuedConnection);
}

Mod::~Mod()
{
    for (ModAdapter* adp : adapters_)
    {
        LOG << "Destroying adapter";
        delete adp;
    }
}

void Mod::threadConstructor()
{
    updateTimer_ = new QTimer(this);
    updateTimer_->setTimerType(Qt::VeryCoarseTimer);
    updateTimer_->setInterval(1000);
}


QString Mod::path() const
{
    return SettingsModel::modDownloadPath() + "/" + name();
}

void Mod::init()
{
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
    QString error = sync_->folderError(key_);
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
    if (sync_->folderExists(key_))
    {
        sync_->setFolderPath(key_, path());
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
    for (QString path : extraFiles)
    {
        LOG << "Deleting extra file " << path << " from mod " << name();
        FileUtils::rmCi(path);
    }
    LOG << "Completed name = " << name();
}

//Returns true if at least one adapter is active.
bool Mod::ticked()
{
    if (!optional() && !reposInactive())
        return true;

    for (ModAdapter* adp : modAdapters())
    {
        if (adp->ticked())
            return true;
    }
    return false;
}

QString Mod::startText()
{
    return "hidden";
}

QString Mod::joinText()
{
    return "hidden";
}

//If all repositories this mod is included in are disabled then stop the
//download.
void Mod::repositoryChanged()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (reposInactive() || !ticked())
    {
        if (sync_->folderExists(key_) && !sync_->folderPaused(key_))
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
}

QSet<Repository*> Mod::repositories() const
{
    return repositories_;
}

QString Mod::key() const
{
    return key_;
}

void Mod::startUpdates()
{
    LOG << name();
    connect(updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
    QMetaObject::invokeMethod(updateTimer_, "start", Qt::QueuedConnection);
}

//This should always be run in UI Thread
void Mod::stopUpdates()
{
    LOG << name();
    //Updates need to be stopped before object destruction, hence blocking.
    QMetaObject::invokeMethod(this, "stopUpdatesSlot", Qt::BlockingQueuedConnection);
    //Process pending updateView request in UI Thread. (Prevents segfault)
    //QCoreApplication::processEvents();
}

//This should always be run in workerThread
void Mod::stopUpdatesSlot()
{
    updateTimer_->stop();
    updateTimer_->disconnect();
}

void Mod::appendRepository(Repository* repository)
{
    LOG << "mod name = " << name() << " repo name = " << repository->name();

    repositories_.insert(repository);
    if (repositories().size() == 1)
    {
        //First repository added -> initialize mod.
        Repository* repo = *repositories_.begin();
        if (sync_->ready())
        {
            LOG << "name = " << name() << " Calling init directly";
            QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
        }
        else
        {
            connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(init()));
            LOG << "name = " << name() << " initCompleted connection created";
        }
    }
    QMetaObject::invokeMethod(this, "repositoryChanged", Qt::QueuedConnection);
}

bool Mod::removeRepository(Repository* repository)
{

    QString errorMsg = QString("ERROR: Mod %1 not found in repository %2.").
            arg(name()).arg(repository->name());

    auto it = repositories_.find(repository);
    if (it == repositories_.end())
    {
        LOG << errorMsg;
        return false;
    }
    repositories_.erase(it);
    for (ModAdapter* adp : adapters_)
    {
        if (adp->parentItem() == repository)
        {
            repository->removeChild(adp);
            LOG << "Mod View Adapter removed.";
            return true;
        }
    }
    LOG << errorMsg;
    return false;
}

//Returns true only if all adapters are optional
bool Mod::optional() const
{
    bool rVal = true;
    for (ModAdapter* adp : modAdapters())
    {
        rVal = rVal && adp->optional();
    }
    return rVal;
}

QVector<ModAdapter*> Mod::modAdapters() const
{
    return adapters_;
}

void Mod::appendModAdapter(ModAdapter* adapter)
{
    adapters_.append(adapter);
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
    else if (sync_->folderQueued(key_))
    {
        setStatus(SyncStatus::QUEUED);
    }
    else if (sync_->folderChecking(key_))
    {
        setStatus(sync_->folderPatching(key_) ? SyncStatus::CHECKING_PATCHES : SyncStatus::CHECKING);
    }
    //Hack to fix BtSync reporting ready when it's not... :D (oh my god...)
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
        else if (sync_->folderNoPeers(key_))
        {
            setStatus(SyncStatus::NO_PEERS);
        }
        else if (sync_->folderPatching(key_))
        {
            setStatus(SyncStatus::PATCHING);
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
    // Hack to manually fix LibTorrent 1.1.7 eternal queue bug
    sync_->disableQueue(key_);
    check();
}

QString Mod::progressStr()
{
    if (!ticked())
        return "???";

    return toProgressStr(totalWanted(), totalWantedDone());
}

QString Mod::bytesToMegasStr(const qint64 bytes)
{
    return QString::number(1 + ((bytes - 1) / Constants::MEGA_DIVIDER));  // Ceil division
}

QString Mod::toProgressStr(const qint64 totalWanted, const qint64 totalWantedDone)
{
    if (totalWanted == -1 || totalWantedDone == -1)
        return "???";

    return QString("%1 / %2").arg(bytesToMegasStr(totalWantedDone)).arg(bytesToMegasStr(totalWanted));
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
    LOG << name() << " checked state set to " << ticked();
    //Below cmd will start the download if repository is active.
    QMetaObject::invokeMethod(this, "repositoryChanged", Qt::QueuedConnection);
}

bool Mod::reposInactive()
{
    for (Repository* repo : repositories_)
    {
        if (repo->statusStr() != SyncStatus::INACTIVE)
            return false;
    }
    return true;
}

void Mod::updateEta()
{
    if (statusStr() == SyncStatus::INACTIVE
            || statusStr() == SyncStatus::ERRORED || statusStr() == SyncStatus::READY)
    {
        setEta(0);
        return;
    }
    setEta(sync_->folderEta(key_));
}

Repository* Mod::parentItem()
{
    return static_cast<Repository*>(TreeItem::parentItem());
}
