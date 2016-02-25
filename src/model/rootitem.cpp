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
#include "api/libbtsync-qt/bts_global.h"
#include "rootitem.h"
#include "repository.h"
#include "jsonreader.h"
#include "settingsmodel.h"
#include "apikey.h"

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
    initBtsync();
    Global::btsync = btsync_;
    DBG << "initBtsync completed";
    JsonReader::fillEverything(this);
    //JsonReader::fillRepositories(this);
    DBG << "readJson completed";
    removeOrphans();
    DBG << "remove orhpans completed";
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
    btsync_->shutdown2();
    DBG << "Btsync destruction";
    delete btsync_;
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
    for (const QString& key : btsync_->getFolderKeys())
    {
        if (!keys.contains(key))
        {
            //Not found
            DBG << "Deleting folder with key:" << key;
            btsync_->removeFolder(key);
        }
    }
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
    btsync_->shutdown2();
    for (Repository* repo : childItems())
    {
        if (repo->checked())
        {
            repo->checkboxClicked(true);
            repo->update(this);
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
    btsync_->restart2();
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
    return btsync_;
}


void RootItem::initBtsync()
{
    DBG;
    #ifdef Q_OS_WIN
        BtsGlobal::setBtsyncExecutablePath("bin/btsync.exe");
    #elif defined(Q_PROCESSOR_X86_32)
        BtsGlobal::setBtsyncExecutablePath("bin/btsync32");
    #else
        BtsGlobal::setBtsyncExecutablePath("bin/btsync");
    #endif
    BtsGlobal::setApiKey(BTS_API_KEY);
    btsync_ = new BtsApi2(createBtsClient());

    updateTimer_.setInterval(1000);
    DBG << "Setting up speed updates";
    QObject::connect(&updateTimer_, SIGNAL(timeout()), btsync_, SLOT(getSpeed()));
    QObject::connect(btsync_, SIGNAL(getSpeedResult(qint64,qint64)), parent_, SLOT(updateSpeed(qint64,qint64)));
    updateTimer_.start();
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

BtsClient* RootItem::createBtsClient()
{
    DBG;
    //BtsSpawnClient* btsclient = new BtsSpawnClient(this);
    BtsSpawnClient* btsclient = new BtsSpawnClient();
    btsclient->setUsername(username_); //TODO: comment
    btsclient->setPassword(password_); //TODO: comment
    btsclient->setPort(port_);
    QString host = "127.0.0.1";
    if (Global::guiless)
    {
        host = "0.0.0.0";
    }
    btsclient->setHost(host);
    btsclient->setAutorestart(false);
    QString dataPath = Constants::BTSYNC_SETTINGS_PATH;
    DBG << "Creating BtSync data path.";
    QDir().mkpath(dataPath);
    btsclient->setDataPath(dataPath);
    DBG << "Starting BtSync";
    btsclient->startClient();

    return btsclient;
}
