#include <sys/types.h>
#include <signal.h>
#include <QThread>
#include "debug.h"
#include <QModelIndex>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTimer>
#include "global.h"
#include "rootitem.h"
#include "repository.h"
#include "jsonreader.h"
#include "settingsmodel.h"

//TODO: Should it be faster?
RootItem::RootItem(TreeModel* parentModel):
    RootItem(Constants::DEFAULT_USERNAME, Constants::DEFAULT_PASSWORD,
             Constants::DEFAULT_PORT, parentModel)
{
}

RootItem::RootItem(const QString& username, const QString& password, unsigned port,
                   TreeModel* parentModel):
    QObject(),
    TreeItem("[DBG] Root Item"),
    initializing_(true),
    parent_(parentModel),
    username_(username),
    password_(password),
    port_(port)
{
    initSync();
    Global::btsync = sync_;
    DBG << "initBtsync completed";
    JsonReader::fillEverything(this);
    DBG << "readJson completed";
    if (sync_->ready())
    {
        removeOrphans();
    }
    else
    {
        connect(sync_, SIGNAL(initCompleted()), this, SLOT(removeOrphans()));
    }
    initializing_ = false;
    Global::workerThread->start();
}

RootItem::~RootItem()
{
    DBG;
    for (Repository* repo : childItems())
    {
        delete repo;
    }
    Global::workerThread->quit();
    Global::workerThread->wait(3000);
    DBG << "Shutdown2";
    sync_->shutdown2();
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
                     << " repository=" << repository->name();
            keys.insert(mod->key());
        }
    }
    for (const QString& key : sync_->getFolderKeys())
    {
        if (!keys.contains(key))
        {
            //Not found
            DBG << "Deleting folder with key:" << key;
            sync_->removeFolder2(key);
        }
    }
    DBG << "remove orhpans completed";
}

void RootItem::processCompletion()
{
    DBG << "Started";
    for (Repository* repo : childItems())
    {
        repo->processCompletion();
    }
    DBG << "Finished";
}

void RootItem::resetSyncSettings()
{
    DBG;
    sync_->shutdown2();
    for (Repository* repo : childItems())
    {
        if (repo->checked())
        {
            repo->checkboxClicked(true);
            repo->updateView(this);
        }
    }
    //Brute way to avoid "file in use" while btsync is shutting down.
    QDir dir(Constants::BTSYNC_SETTINGS_PATH);
    int attempts = 0;
    while ( dir.exists() && attempts < 100 )
    {
        dir.removeRecursively();
        //It's rape time.
        DBG << "Warning: Failure to delete BtSync Storage... retrying path ="
            << dir.currentPath()
            << " attempts =" << attempts;
        QThread::sleep(1);
        ++attempts;
    }
    DBG << "Sync storage deleted. path =" << dir.currentPath();
    dir.mkpath(".");
    sync_->restart2();
}

QList<Repository*> RootItem::childItems()
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
    //DBG << "Updating row=" << row->row();
    parent_->updateView(item, row);
}

BtsApi2* RootItem::btsync() const
{
    return sync_;
}

void RootItem::enableRepositories()
{
    DBG;
    for (Repository* repo : childItems())
    {
        if (!repo->checked())
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

void RootItem::initSync()
{
    sync_ = new BtsApi2(username_, password_, port_);
    updateTimer_.setInterval(1000);
    DBG << "Setting up speed updates";
    connect(&updateTimer_, SIGNAL(timeout()), sync_, SLOT(getSpeed()));
    connect(sync_, SIGNAL(getSpeedResult(qint64,qint64)), parent_, SLOT(updateSpeed(qint64,qint64)));
    if (sync_->ready())
    {
        DBG << "Starting updateTimer directly";
        updateTimer_.start();
    }
    else
    {
        DBG << "Connecting updateTimer start to initCompleted()";
        connect(sync_, SIGNAL(initCompleted()), &updateTimer_, SLOT(start()));
    }
}
