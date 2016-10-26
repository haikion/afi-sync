#include <sys/types.h>
#include <signal.h>
#include <QThread>
#include <QModelIndex>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTimer>
#include "apis/libtorrent/libtorrentapi.h"
#include "debug.h"
#include "global.h"
#include "rootitem.h"
#include "repository.h"
#include "jsonreader.h"
#include "settingsmodel.h"
#include "processmonitor.h"
#include "fileutils.h"

const int RootItem::REPO_UPDATE_DELAY = 60000; //ms

RootItem::RootItem(TreeModel* parentModel):
    RootItem(Constants::DEFAULT_PORT, parentModel)
{
}

RootItem::RootItem(unsigned port,
                   TreeModel* parentModel):
    QObject(),
    TreeItem("[DBG] Root Item"),
    initializing_(true),
    parent_(parentModel),
    port_(port)
{
    initSync();
    Global::sync = sync_;
    DBG << "initSync completed";
    jsonReader_.fillEverything(this);
    DBG << "readJson completed";
    if (sync_->ready())
    {
        removeOrphans();
    }
    else
    {
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(removeOrphans()));
    }
    initializing_ = false;
    Global::workerThread->setObjectName("workerThread");
    Global::workerThread->start();
    DBG << "Worker thread started";
}

RootItem::~RootItem()
{
    DBG;
    //Causes segfaults (because of workerThread->quit()?) and there is no need for this anyway
    //if destructor is only used during program shutdown.
    //for (Repository* repo : childItems())
    //{
    //    delete repo;
    //}
    stopUpdates();
    Global::workerThread->quit();
    Global::workerThread->wait(3000);
    DBG << "Shutdown";
    sync_->shutdown();
    DBG << "Shutdown done";
    //Causes chrashes if BtSync connection is unestablished.
    //delete sync_;
}

//Removes btsync dirs which
//are not represented in this program.
void RootItem::removeOrphans()
{
    QSet<QString> keys;

    for (const Repository* repository : childItems())
    {
        for (const Mod* mod : repository->mods())
        {
            DBG << "Processing mod =" << mod->name()
                     << " key =" << mod->key()
                     << " repository =" << repository->name();
            keys.insert(mod->key());
        }
    }
    for (const QString& key : sync_->folderKeys())
    {
        if (!keys.contains(key))
        {
            //Not found
            DBG << "Deleting folder with key:" << key;
            sync_->removeFolder(key);
        }
    }
    DBG << "Done";
}

void RootItem::processCompletion()
{
    DBG << "Started";
    for (Repository* repo : childItems())
    {
        if (repo->status() == SyncStatus::INACTIVE)
        {
            DBG << "Skipping" << repo->name() << "due to inactivity.";
            continue;
        }
        repo->processCompletion();
    }
    DBG << "Finished";
}

void RootItem::rowsChanged()
{
    //Run only in GUI thread.
    QMetaObject::invokeMethod(parent_, "rowsChanged", Qt::QueuedConnection);
}

bool RootItem::stopUpdates()
{
    parent_->setHaltGui(true);
    bool rVal = updateTimer_.isActive() && repoTimer_.isActive();
    updateTimer_.stop();
    repoTimer_.stop();
    for (Repository* repo : childItems())
    {
        repo->stopUpdates();
    }
    parent_->setHaltGui(false);
    return rVal;
}

void RootItem::startUpdateTimers()
{
    if (sync_->ready())
    {
        DBG << "Starting updateTimer directly";
        updateTimer_.start();
        repoTimer_.start();
    }
    else
    {
        DBG << "Connecting updateTimer start to initCompleted()";
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), &updateTimer_, SLOT(start()), Qt::UniqueConnection);
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), &repoTimer_, SLOT(start()), Qt::UniqueConnection);
    }
}

void RootItem::setDeltaUpdatesKey(const QString& key)
{
    sync_->setDeltaUpdatesFolder(key, SettingsModel::modDownloadPath());
    deltaUpdateKey_ = key;
}

QString RootItem::deltaUpdatesKey() const
{
    return sync_->deltaUpdatesKey();
}

void RootItem::startUpdates()
{
    //startUpdateTimers(); Called by repo if needed
    for (Repository* repo : childItems())
    {
        if (repo->status() == SyncStatus::INACTIVE)
            continue;

        repo->startUpdates();
    }
}

void RootItem::resetSyncSettings()
{
    DBG;
    for (Repository* repo : childItems())
    {
        if (repo->ticked())
        {
            //Disable repo
            repo->checkboxClicked(true);
            //Display changes.
            repo->updateView(this);
        }
    }
    sync_->shutdown();
    //Brute way to avoid "file in use" while sync is shutting down.
    QDir dir(Constants::SYNC_SETTINGS_PATH);
    int attempts = 0;
    while ( dir.exists() && attempts < 100 )
    {
        FileUtils::safeRemoveRecursively(dir);
        //It's rape time.
        DBG << "Warning: Failure to delete Sync Storage... retrying path ="
            << dir.absolutePath() << "attempts =" << attempts;
        QThread::sleep(1);
        ++attempts;
    }
    DBG << "Sync storage deleted. path =" << dir.absolutePath();
    dir.mkpath(".");
    sync_->start();
}

QList<Repository*> RootItem::childItems() const
{
    QList<Repository*> rVal;
    for (TreeItem* item : TreeItem::childItems())
    {
        rVal.append(static_cast<Repository*>(item));
    }
    return rVal;
}

void RootItem::updateView(TreeItem* item, int row)
{
    if (initializing_)
    {
        //Wait for repos to load
        return;
    }
    //DBG << "Updating row =" << row->row();
    parent_->updateView(item, row);
}

ISync* RootItem::sync() const
{
    return sync_;
}

void RootItem::enableRepositories()
{
    DBG;
    for (Repository* repo : childItems())
    {
        if (!repo->ticked())
        {
            repo->checkboxClicked();
        }
        repo->enableMods();
    }
}

QString RootItem::defaultter(const QString& value, const QString& defaultValue)
{
    if (value == "")
    {
        return defaultValue;
    }
    return value;
}

void RootItem::updateSpeed()
{
    if (!parent_ || !sync_)
    {
        DBG << "ERROR: null value. parent_ =" << parent_ << "sync_" << sync_;
        return;
    }

    parent_->updateSpeed(sync_->download(), sync_->upload());
}

void RootItem::periodicRepoUpdate()
{
    for (Repository* repo : childItems())
    {
        if (repo->status() == SyncStatus::PATCHING)
        {
            DBG << "Periodic update disabled while patching.";
            return;
        }
    }
    if (ProcessMonitor::afiSyncRunning())
    {
        DBG << "Ignored because Arma 3 is running";
        return;
    }
    if (jsonReader_.updateAvaible())
    {
        DBG << "Updating repo...";
        jsonReader_.fillEverything(this);
        return;
    }
    DBG << "No repo updates";
}

void RootItem::update()
{
    updateSpeed();
    for (Repository* repo : childItems())
    {
        repo->update();
    }
}

void RootItem::initSync()
{
    sync_ = new LibTorrentApi();
    updateTimer_.setInterval(1000);
    repoTimer_.setInterval(REPO_UPDATE_DELAY);
    DBG << "Setting up speed updates";
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
    connect(&repoTimer_, SIGNAL(timeout()), this, SLOT(periodicRepoUpdate()));
}
