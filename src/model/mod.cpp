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
#include "modviewadapter.h"

const unsigned Mod::COMPLETION_WAIT_DURATION = 0;

Mod::Mod(const QString& name, const QString& key, bool isOptional):
    SyncItem(name, 0),
    isOptional_(isOptional),
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
    for (ModViewAdapter* adp : viewAdapters_)
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

    if (!isOptional_)
    {
        setChecked(true);
    }
    //Periodically check mod progress
    if (!updateTimer_)
    {
        updateTimer_ = new QTimer();
    }
    updateTimer_->setInterval(1000);
    connect(updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
    repositoryEnableChanged();
    update();
    DBG << "name =" << name() << "key =" << key() << "Completed";
}

void Mod::update()
{
    updateStatus();
    fetchEta();
    for (ModViewAdapter* adp : viewAdapters_)
    {
        adp->updateView();
    }
}

//Starts sync directory. If directory does not exist
//directory will be created. If path is incorrect
//directory will be re-created correctly.
void Mod::start()
{
    DBG << "name =" << name();
    QString modPath = SettingsModel::modDownloadPath();
    QString dir = QDir::toNativeSeparators(modPath);
    QString syncPath = QDir::toNativeSeparators(sync_->getFolderPath(key_));
    QString error = sync_->error(key_);

    if (syncPath.toUpper() != dir.toUpper() || error != "")
    {
        DBG << "name =" << name() << "Re-adding directory. syncPath ="
            << syncPath << "dir =" << dir
            << "error =" << error;
        //Disagreement between Sync and AfiSync
        sync_->removeFolder2(key_);
        sync_->addFolder(dir, key_, true);
    }
    sync_->setFolderPaused(key_, false);
    QMetaObject::invokeMethod(updateTimer_, "start", Qt::QueuedConnection);
}

void Mod::deleteExtraFiles()
{
    DBG << "name =" << name();
    if (!checked())
    {
        DBG << "name =" << name()  << "Mod is inactive, doing nothing.";
        return;
    }

    QSet<QString> localFiles;
    QSet<QString> remoteFiles = sync_->getFilesUpper(key_);

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
        DBG << "name =" << name() << "Warning: Not deleting extra files because mod is not fully synced.";
        return; //Would delete everything otherwise
    }

    DBG << " name =" << name()
             << "\n\n remoteFiles =" << remoteFiles
             << "\n\n localFiles =" << localFiles;

    QSet<QString> extraFiles = localFiles - remoteFiles;
    for (QString file : extraFiles)
    {
        DBG << "Deleting extra file: " << file << " from mod =" << name();
        //QFile(file).remove();
    }
    DBG << "Completed name =" << name();
}

bool Mod::checked() const
{
    if (!isOptional_)
    {
        return true;
    }
    return SyncItem::checked();
}

QString Mod::checkText()
{
    if (!isOptional_)
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
void Mod::repositoryEnableChanged(bool offline)
{
    DBG << "name =" << name() << "current thread: " << QThread::currentThread() << "Worker thread: " << Global::workerThread;
    //just in case
    if (!sync_)
        return;

    bool allDisabled = true;
    for (const Repository* repo : repositories_)
    {
        DBG <<"Checking mod name =" << name() << "repo name =" << repo->name();
        if (repo->checked())
        {
            //At least one repo is active
            allDisabled = false;
            break;
        }
    }
    if (offline)
    {
        DBG << "Offline, not updating Sync";
        return;
    }
    if (allDisabled || !checked())
    {
        //All repositories unchecked
        DBG << "Stopping mod transfer. name =" << name();
        sync_->setFolderPaused(key_, true);
        QMetaObject::invokeMethod(updateTimer_, "stop", Qt::QueuedConnection);
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
    QMetaObject::invokeMethod(updateTimer_, "start", Qt::QueuedConnection);
}

void Mod::stopUpdates()
{
    QMetaObject::invokeMethod(updateTimer_, "stop", Qt::QueuedConnection);
}

void Mod::addRepository(Repository* repository)
{
    DBG << "mod name =" << name() << " repo name =" << repository->name();

    repositories_.insert(repository);
    if (repositories().size() == 1)
    {
        Repository* repo = *repositories_.begin();
        sync_ = repo->sync();
        if (sync_->folderReady())
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
}

bool Mod::removeRepository(Repository* repository)
{
    DBG;

    //disconnect(repository, SIGNAL(enableChanged()), this, SLOT(repositoryEnableChanged()));
    auto it = repositories_.find(repository);
    if (it == repositories_.end())
    {
        DBG << "ERROR: Mod" << name() << "not found in repository:" << repository->name();
        return false;
    }
    repositories_.erase(it);
    return true;
}

QVector<ModViewAdapter*> Mod::viewAdapters() const
{
    return viewAdapters_;
}

void Mod::addModViewAdapter(ModViewAdapter* adapter)
{
    viewAdapters_.append(adapter);
}

void Mod::updateStatus()
{
    QString processKey = name() + "/process";

    if (!checked())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if (!sync_->exists(key_))
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
    else if (sync_->paused(key_))
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
    else if (sync_->noPeers(key_))
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
    sync_->check(key_);
    deleteExtraFiles();
    Installer::install(this);
}

void Mod::checkboxClicked()
{
    SyncItem::checkboxClicked();
    DBG << "name =" << name() << "checked()=" << checked();
    QMetaObject::invokeMethod(this, "repositoryEnableChanged", Qt::QueuedConnection);
}

void Mod::fetchEta()
{
    if (status() == SyncStatus::INACTIVE)
    {
        setEta(0);
        return;
    }
    int eta = sync_->getFolderEta(key_);
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
