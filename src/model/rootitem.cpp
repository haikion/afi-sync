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
    QTimer::singleShot(15000, this, SLOT(removeOrphans()));
    initializing_ = false;
}

RootItem::~RootItem()
{
    DBG;
    for (Repository* repo : childItems())
    {
        delete repo;
    }
    DBG << "Shutdown2";
    sync_->shutdown2();
    DBG << "Btsync destruction";
    delete sync_;
}

//Removes btsync dirs which
//are not represented in this program.
void RootItem::removeOrphans()
{
    QSet<QString> keys;

    for (const Repository* repository : childItems())
    {
        for (const Mod* mod : repository->childItems())
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
        DBG << "Failure to delete BtSync Storage... retrying path=" << dir.currentPath()
            << " attempts =" << attempts;
        QThread::sleep(1);
        ++attempts;
    }
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
    QObject::connect(&updateTimer_, SIGNAL(timeout()), sync_, SLOT(getSpeed()));
    QObject::connect(sync_, SIGNAL(getSpeedResult(qint64,qint64)), parent_, SLOT(updateSpeed(qint64,qint64)));
    updateTimer_.start();
}
