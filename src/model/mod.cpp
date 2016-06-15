#include "debug.h"
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QEventLoop>
#include <QDirIterator>
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
    sync_(0),
    updateTimer_(nullptr),
    waitTime_(0)
{
    DBG;
    setStatus(SyncStatus::NO_SYNC_CONNECTION);
    moveToThread(Global::workerThread);
    qRegisterMetaType<QVector<int>>("QVector<int>");
}

Mod::~Mod()
{
    DBG;
    for (ModAdapter* adp : adapters_)
    {
        DBG << "Destroying adapter";
        delete adp;
    }
    QMetaObject::invokeMethod(this, "threadDestructor", Qt::QueuedConnection);
}

void Mod::threadDestructor()
{
    delete updateTimer_;
}

void Mod::init()
{
    DBG << "name =" << name() << "current thread:" << QThread::currentThread() << "Worker thread:" << Global::workerThread;

    if (!isOptional())
    {
        setTicked(true);
    }
    //Periodically check mod progress
    if (!updateTimer_)
    {
        updateTimer_ = new QTimer();
    }
    updateTimer_->setInterval(1000);
    connect(updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
    repositoryChanged();
    update();
    DBG << "name =" << name() << "key =" << key() << "Completed";
}

void Mod::update()
{
    updateStatus();
    fetchEta();
    for (ModAdapter* adp : adapters_)
    {
        adp->updateView();
    }
}

//Starts sync directory. If directory does not exist
//folder will be created. If path is incorrect
//folder will be re-created correctly.
//If mod is not in sync it will be added to it.
void Mod::start()
{
    DBG << "name =" << name();
    QString modPath = SettingsModel::modDownloadPath();
    QString dir = QDir::toNativeSeparators(modPath);

    if (!sync_->folderExists(key_))
    {
        //Add folder
        DBG << "Adding" << name() << "to sync.";
        sync_->addFolder(dir, key_);
    }

    //Sanity checks
    QString syncPath = QDir::toNativeSeparators(sync_->folderPath(key_));
    QString error = sync_->folderError(key_);
    if (syncPath.toUpper() != dir.toUpper() || error != "")
    {
        DBG << "name =" << name() << "Re-adding directory. syncPath ="
            << syncPath << "dir =" << dir
            << "error =" << error;
        //Disagreement between Sync and AfiSync
        sync_->removeFolder(key_);
        sync_->addFolder(key_, dir);
    }

    //Do the actual starting
    sync_->setFolderPaused(key_, false);
    startUpdates();
}

bool Mod::stop()
{
    DBG;
    if (!sync_->folderExists(key_))
        return false;

    if (!sync_->folderPaused(key_))
        return false;

    DBG << "Stopping mod transfer. name =" << name();
    sync_->setFolderPaused(key_, true);
    QMetaObject::invokeMethod(updateTimer_, "stop", Qt::QueuedConnection);
    update();
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
    //In case of incorrect configuration... or no peers (thx bts)
    //or ..bts sends incorrect files list...
    if (remoteFiles.size() == 0 || status() != SyncStatus::READY)
    {
        DBG << "name =" << name()
            << "Warning: Not deleting extra files because mod is not fully synced.";
        return; //Would delete everything otherwise
    }

    DBG << " name =" << name()
             << "\n\n remoteFiles =" << remoteFiles
             << "\n\n localFiles =" << localFiles;

    QSet<QString> extraFiles = localFiles - remoteFiles;
    for (QString file : extraFiles)
    {
        DBG << "Deleting extra file" << file << "from mod" << name();
        QFile(file).remove();
    }
    DBG << "Completed name =" << name();
}

//Returns true if at least one adapter is active.
bool Mod::ticked() const
{
    if (!isOptional() && !reposInactive())
    {
        return true;
    }
    for (ModAdapter* adp : modAdapters())
    {
        if (adp->ticked())
            return true;
    }
    return false;
}

QString Mod::checkText()
{
    if (!isOptional())
    {
        return "disabled";
    }
    return SyncItem::checkText();
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
    DBG << "name =" << name() << "current thread:"
        << QThread::currentThread() << "Worker thread:" << Global::workerThread;
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
        DBG << "All repositories inactive or mod unchecked. Stopping" << name();
        stop();
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
    QMetaObject::invokeMethod(updateTimer_, "start", Qt::QueuedConnection);
}

void Mod::stopUpdates()
{
    DBG << name();
    QMetaObject::invokeMethod(updateTimer_, "stop", Qt::QueuedConnection);
}

void Mod::appendRepository(Repository* repository)
{
    DBG << "mod name =" << name() << " repo name =" << repository->name();

    repositories_.insert(repository);
    if (repositories().size() == 1)
    {
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
    repositoryChanged();
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
    QString processKey = name() + "/process";

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
            setStatus(SyncStatus::READY);
        }
    }
    else if (status() == SyncStatus::READY || status() == SyncStatus::READY_PAUSED)
    {
        bool process = settings()->value(processKey, true).toBool();
        if (process)
        {
            processCompletion();
            DBG << "Process set to false";
            settings()->setValue(processKey, false);
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
    else if (eta() > 0)
    {
        setStatus(SyncStatus::DOWNLOADING);
        DBG << "process set to true";
        settings()->setValue(processKey, true);
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

void Mod::fetchEta()
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
