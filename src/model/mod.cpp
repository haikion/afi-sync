#include "debug.h"
#include <QTimer>
#include <QDirIterator>
#include <QCoreApplication>
#include "fileutils.h"
#include "global.h"
#include "settingsmodel.h"
#include "mod.h"
#include "repository.h"
#include "installer.h"
#include "modadapter.h"

const unsigned Mod::COMPLETION_WAIT_DURATION = 0;

Mod::Mod(const QString& name, const QString& key):
    SyncItem(name, 0),
    key_(key),
    sync_(nullptr),
    updateTimer_(nullptr),
    waitTime_(0),
    processCompletionKey_(name + "/process")
{
    DBG;
    setStatus(SyncStatus::NO_SYNC_CONNECTION);
    //Enables non lagging UI
    moveToThread(Global::workerThread);
    qRegisterMetaType<QVector<int>>("QVector<int>");
    //Blocking is required, because otherwise startUpdates might be called before updateTimer_ is initialized.
    QMetaObject::invokeMethod(this, "threadConstructor", Qt::BlockingQueuedConnection);
}

Mod::~Mod()
{
    DBG;
    for (ModAdapter* adp : adapters_)
    {
        DBG << "Destroying adapter";
        delete adp;
    }
}

void Mod::threadConstructor()
{
    updateTimer_ = new QTimer(this);
    updateTimer_->setTimerType(Qt::VeryCoarseTimer);
    updateTimer_->setInterval(1000);
}

void Mod::init()
{
    if (!isOptional())
        setTicked(true);

    repositoryChanged();
    update();
    DBG << "name =" << name() << "key =" << key() << "Completed";
}

void Mod::update(bool force)
{
    updateStatus();
    updateEta();
    updateView(force);
}

void Mod::updateView(bool force)
{
    for (ModAdapter* adp : adapters_)
    {
        //Only update if mod and its repo is active or when force == true
        if ((adp->ticked() && adp->repo()->ticked()) || force)
        {
            //Run in main (UI) thread.
            QMetaObject::invokeMethod(adp, "updateView", Qt::QueuedConnection);
        }
    }
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
            DBG << "Removing conflicting (" << syncKey << "," << syncPath << ") for (" << key_ << "," << path() << ").";
            sync_->removeFolder(syncKey);
            return;
        }
    }
}

QString Mod::path() const
{
    return SettingsModel::modDownloadPath() + "/" + name();
}

//Starts sync directory. If directory does not exist
//folder will be created. If path is incorrect
//folder will be re-created correctly.
//If mod is not in sync it will be added to it.
void Mod::start()
{
    DBG << "name =" << name();

    if (!sync_->folderExists(key_))
    {
        removeConflicting();
        //Add folder
        DBG << "Adding" << name() << "to sync.";
        sync_->addFolder(key_, name());
        setProcessCompletion(true);
    }
    //Sanity checks
    QString error = sync_->folderError(key_);
    if (!error.isEmpty())
    {
        DBG << "name =" << name() << "Re-adding directory. error =" << error;
        //Disagreement between Sync and AFISync
        sync_->removeFolder(key_);
        sync_->addFolder(key_, name());
        setProcessCompletion(true);
    }
    //Do the actual starting
    sync_->setFolderPaused(key_, false);
    startUpdates();
}

void Mod::setProcessCompletion(bool value)
{
    settings()->setValue(processCompletionKey_, value);
    DBG << "Process (completion) set to" << value << "for mod" << name();
}

bool Mod::getProcessCompletion() const
{
    return settings()->value(processCompletionKey_, true).toBool();
}

bool Mod::stop()
{
    DBG << name();
    if (!sync_->folderExists(key_))
    {
        DBG << "ERROR: Folder" << name() << "does not exist.";
        return false;
    }

    DBG << "Stopping mod transfer. name =" << name();
    sync_->setFolderPaused(key_, true);
    stopUpdatesSlot();
    update(true);

    return true;
}

void Mod::deleteExtraFiles()
{
    DBG << "name =" << name();
    if (!ticked())
    {
        DBG << "name =" << name()  << "Mod is inactive, doing nothing.";
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
        DBG << "ERROR: Not deleting extra files because torrent contains 0 files.";
        return; //Would delete everything otherwise
    }

    DBG << " name =" << name()
             << "\n\n remoteFiles =" << remoteFiles
             << "\n\n localFiles =" << localFiles;

    QSet<QString> extraFiles = localFiles - remoteFiles;
    for (QString path : extraFiles)
    {
        DBG << "Deleting extra file" << path << "from mod" << name();
        FileUtils::rmCi(path);
    }
    DBG << "Completed name =" << name();
}

//Returns true if at least one adapter is active.
bool Mod::ticked() const
{
    if (!isOptional() && !reposInactive())
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
void Mod::repositoryChanged(bool offline)
{
    //just in case
    if (!sync_)
    {
        DBG << "ERROR: Sync is null" << name();
        return;
    }

    if (offline)
    {
        DBG << "Offline, not updating Sync" << name();
        return;
    }
    if (reposInactive() || !ticked())
    {
        if (sync_->folderExists(key_))
        {
            DBG << "All repositories inactive or mod unchecked. Stopping" << name();
            stop();
        }
        return;
    }
    //At least one repo active and mod checked
    DBG << "Starting mod transfer. name =" << name();
    start();
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
    DBG << name();
    connect(updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
    QMetaObject::invokeMethod(updateTimer_, "start", Qt::QueuedConnection);
}

//This should always be run in UI Thread
void Mod::stopUpdates()
{
    DBG << name();
    DBG << Global::workerThread->isRunning();
    //Updates need to be stopped before object destruction, hence blocking.
    QMetaObject::invokeMethod(this, "stopUpdatesSlot", Qt::BlockingQueuedConnection);
    //Process pending updateView request in UI Thread. (Prevents segfault)
    QCoreApplication::processEvents();
}

//This should always be run in workerThread
void Mod::stopUpdatesSlot()
{
    updateTimer_->stop();
    updateTimer_->disconnect();
}

void Mod::appendRepository(Repository* repository)
{
    DBG << "mod name =" << name() << " repo name =" << repository->name();

    repositories_.insert(repository);
    if (repositories().size() == 1)
    {
        //First repository added -> initialize mod.
        Repository* repo = *repositories_.begin();
        sync_ = repo->sync();
        if (sync_->ready())
        {
            DBG << "name =" << name() << "Calling init directly";
            QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
        }
        else
        {
            connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(init()));
            DBG << "name =" << name() << "initCompleted connection created";
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
        DBG << errorMsg;
        return false;
    }
    repositories_.erase(it);
    for (ModAdapter* adp : adapters_)
    {
        if (adp->parentItem() == repository)
        {
            repository->removeChild(adp);
            DBG << "Mod View Adapter removed.";
            return true;
        }
    }
    DBG << errorMsg;
    return false;
}

//Returns true only if all adapters are optional
bool Mod::isOptional() const
{
    bool rVal = true;
    for (ModAdapter* adp : modAdapters())
    {
        rVal = rVal && adp->isOptional();
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
        setStatus(SyncStatus::NOT_IN_SYNC);
    }
    else if (sync_->folderQueued(key_))
    {
        setStatus(SyncStatus::QUEUED);
    }
    else if (sync_->folderChecking(key_))
    {
        setStatus(SyncStatus::CHECKING);
    }
    //Hack to fix BtSync reporting ready when it's not... :D (oh my god...)
    else if (status() == SyncStatus::WAITING)
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
    else if (sync_->folderReady(key_))
    {
        setStatus(SyncStatus::WAITING);
    }
    else if (sync_->folderPaused(key_))
    {
        if (status() == SyncStatus::READY)
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
    else if (sync_->folderDownloadingPatches(key_))
    {
        setStatus(SyncStatus::DOWNLOADING_PATCHES);
    }
    else if (sync_->folderPatching(key_))
    {
        setStatus(SyncStatus::PATCHING);
    }
    else if (eta() > 0)
    {
        setStatus(SyncStatus::DOWNLOADING);
        setProcessCompletion(true);
    }
}

void Mod::processCompletion()
{
    sync_->checkFolder(key_);
    deleteExtraFiles();
    Installer::install(this);
}

void Mod::checkboxClicked()
{
    //Activate download when mod is active in at least on repo.
    bool allDisabled = false;
    for (ModAdapter* adp : adapters_)
    {
        allDisabled = allDisabled || adp->ticked();
    }
    setTicked(allDisabled);
    DBG << name() << "checked state set to" << ticked();
    //Below cmd will start the download if repository is active.
    QMetaObject::invokeMethod(this, "repositoryChanged", Qt::QueuedConnection);
}

bool Mod::reposInactive() const
{
    for (Repository* repo : repositories_)
    {
        if (repo->status() != SyncStatus::INACTIVE)
            return false;
    }
    return true;
}

void Mod::updateEta()
{
    if (status() == SyncStatus::INACTIVE || status() == SyncStatus::NOT_IN_SYNC)
    {
        setEta(0);
        return;
    }
    int eta = sync_->folderEta(key_);
    if (eta == -404)
    {
        DBG << "ERROR: Unable to retrieve eta for mod" << name();
        return;
    }
    setEta(eta);
}

Repository* Mod::parentItem()
{
    return static_cast<Repository*>(TreeItem::parentItem());
}
