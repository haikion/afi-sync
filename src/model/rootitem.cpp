#include <signal.h>
#include <sys/types.h>
#include "apis/libtorrent/libtorrentapi.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFile>
#include <QModelIndex>
#include <QSet>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include "afisynclogger.h"
#include "fileutils.h"
#include "global.h"
#include "jsonreader.h"
#include "processmonitor.h"
#include "repository.h"
#include "rootitem.h"
#include "settingsmodel.h"

const int RootItem::REPO_UPDATE_DELAY = 60000; //ms

RootItem::RootItem(TreeModel* parentModel):
    QObject(),
    TreeItem("[DBG] Root Item"),
    initializing_(true),
    parent_(parentModel)
{
    //Create worker thread
    Global::workerThread = new QThread();
    Global::workerThread->setObjectName("workerThread");
    Global::workerThread->start();
    LOG << "Worker thread started";

    initSync();
    Global::sync = sync_;
    LOG << "initSync completed";
    printDeletables();
    LOG << "readJson completed";
    if (sync_->ready())
    {
        removeOrphans();
    }
    else
    {
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(removeOrphans()));
    }
    initializing_ = false;
    startUpdates();
}

RootItem::~RootItem()
{
    LOG;
    stopUpdates();
    LOG << "Updates stopped";
    //Causes segfaults (because of workerThread->quit()?) and there is no need for this anyway
    //if destructor is only used during program shutdown.
    for (Repository* repo : childItems())
    {
        LOG << "Deleting repository " << repo->name();
        delete repo;
    }
    LOG << "Repositories deleted";
    Global::workerThread->quit();
    Global::workerThread->wait(1000);
    Global::workerThread->terminate();
    Global::workerThread->wait(1000);
    LOG << "Worker thread shutdown. isFinished() = " << Global::workerThread->isFinished();
    delete Global::workerThread;
    Global::workerThread = nullptr;
    LOG << "Worker thread deleted";
    delete sync_;
    sync_ = nullptr;
    Global::sync = nullptr;
    LOG << "Sync deleted";
}

void RootItem::printDeletables()
{
    QStringList afisyncMods;
    QStringList inactiveMods;

    for (Repository* repo : childItems())
    {
        for (Mod* mod : repo->mods())
        {
            afisyncMods.append(mod->name());
            //FIXME: Incorrect!
            if (!mod->ticked())
            {
                inactiveMods.append(mod->name());
            }
        }
    }

    QString notIncludedStr;
    qint64 spaceNotIncluded = 0;
    QString inactivesStr;
    qint64 spaceInactive = 0;
    static const QString DELETE_CMD = "rmdir";
    QDirIterator it(SettingsModel::modDownloadPath());
    while (it.hasNext())
    {
        QFileInfo f(it.next());
        QString fileName = f.fileName();
        if (!fileName.startsWith("@"))
        {
            continue;
        }

        if (!afisyncMods.contains(fileName))
        {
            notIncludedStr += " " + fileName;
            spaceNotIncluded += FileUtils::dirSize(f.filePath());
        }
        if (!inactiveMods.contains(fileName))
        {
            inactivesStr += " " + fileName;
            spaceInactive += FileUtils::dirSize(f.filePath());
        }
    }

    LOG << "Delete non included mods (Space used: " + QString::number(spaceNotIncluded/1000000000) + " GB ) " + DELETE_CMD + notIncludedStr;
    LOG << "Delete inactive mods (Space used: " + QString::number(spaceInactive/1000000000) + " GB ) " + DELETE_CMD + inactivesStr;
}

//Removes sync dirs which
//are not active in any repository in this program.
void RootItem::removeOrphans()
{
    QSet<QString> keys;

    for (const Repository* repository : childItems())
    {
        for (const Mod* mod : repository->mods())
        {
            LOG << "Processing mod = " << mod->name()
                     << " key = " << mod->key()
                     << " repository = " << repository->name();
            keys.insert(mod->key());
        }
    }
    for (const QString& key : sync_->folderKeys())
    {
        if (!keys.contains(key))
        {
            //Not found
            LOG << "Deleting folder with key: " << key;
            sync_->removeFolder(key);
        }
    }
    LOG << "Done";
}

void RootItem::processCompletion()
{
    LOG << "Started";
    for (Repository* repo : childItems())
    {
        if (repo->statusStr() == SyncStatus::INACTIVE)
        {
            LOG << "Skipping " << repo->name() << " due to inactivity.";
            continue;
        }
        repo->processCompletion();
    }
    LOG << "Finished";
}

void RootItem::rowsChanged()
{
    //Run only in GUI thread.
    parent_->rowsChanged();
}

bool RootItem::stopUpdates()
{
    parent_->setHaltGui(true);
    bool rVal = updateTimer_.isActive() && repoTimer_.isActive();
    updateTimer_.stop();
    repoTimer_.stop();
    for (Repository* repo : childItems())
        repo->stopUpdates();

    return rVal;
}

void RootItem::startUpdateTimers()
{
    if (sync_->ready())
    {
        LOG << "Starting updateTimer directly";
        updateTimer_.start();
        repoTimer_.start();
    }
    else
    {
        LOG << "Connecting updateTimer start to initCompleted()";
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), &updateTimer_, SLOT(start()), Qt::UniqueConnection);
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), &repoTimer_, SLOT(start()), Qt::UniqueConnection);
    }
}

void RootItem::startUpdates()
{
    startUpdateTimers();
    for (Repository* repo : childItems())
    {
        if (repo->statusStr() == SyncStatus::INACTIVE)
            continue;

        repo->startUpdates();
    }
    parent_->setHaltGui(false);
}

void RootItem::resetSyncSettings()
{
    LOG;
    for (Repository* repo : childItems())
    {
        if (repo->ticked())
        {
            //Disable repo
            //repo->checkboxClicked(true);
        }
    }
    sync_->shutdown();
    //Brute way to avoid "file in use" while sync is shutting down.
    QDir dir(SettingsModel::syncSettingsPath());
    int attempts = 0;
    while ( dir.exists() && attempts < 100 )
    {
        FileUtils::safeRemoveRecursively(dir);
        //It's rape time.
        LOG_WARNING << "Failure to delete Sync Storage... retrying path = "
            << dir.absolutePath() << " attempts = " << attempts;
        QThread::sleep(1);
        ++attempts;
    }
    LOG << "Sync storage deleted. path = " << dir.absolutePath();
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
    if (initializing_ || parent_ == nullptr)
    {
        //Wait for repos to load
        return;
    }
    //parent_->updateView(item, row);
}

ISync* RootItem::sync() const
{
    return sync_;
}

void RootItem::enableRepositories()
{
    LOG;
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
    if (value.isEmpty())
        return defaultValue;

    return value;
}

void RootItem::updateSpeed()
{
    if (!parent_ || !sync_)
    {
        LOG_ERROR << "Null value. parent_ = " << parent_ << " sync_ = " << sync_;
        return;
    }

//    parent_->updateSpeed(sync_->download(), sync_->upload());
}

//FIXME
void RootItem::periodicRepoUpdate()
{
    for (Repository* repo : childItems())
    {
        if (repo->statusStr() == SyncStatus::PATCHING)
        {
            LOG << "Periodic update disabled while patching.";
            return;
        }
    }
    if (ProcessMonitor::afiSyncRunning())
    {
        LOG << "Ignored because Arma 3 is running";
        return;
    }
    if (jsonReader_.updateAvailable())
    {
        LOG << "Updating repo...";
        //jsonReader_.fillEverything(this);
        return;
    }
    LOG << "No repo updates";
}

void RootItem::update()
{
    updateSpeed();
    for (Repository* repo : childItems())
    {
        if (repo->ticked())
            repo->update();
    }
}

void RootItem::initSync()
{
    sync_ = new LibTorrentApi();
    updateTimer_.setInterval(1000);
    repoTimer_.setInterval(REPO_UPDATE_DELAY);
    LOG << "Setting up speed updates";
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
    connect(&repoTimer_, SIGNAL(timeout()), this, SLOT(periodicRepoUpdate()));
}
