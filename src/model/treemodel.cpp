/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include <sys/types.h>
#include <signal.h>
#include "afisynclogger.h"
#include <QDir>
#include <QStringList>
#include <QFile>
#include <QCoreApplication>
#include "treeitem.h"
#include "repository.h"
#include "mod.h"
#include "treemodel.h"
#include "global.h"
#include "modadapter.h"
#include "deletabledetector.h"
#include "settingsmodel.h"

TreeModel::TreeModel(QObject* parent, ISync* sync, bool haltGui):
    QObject(parent),
    download_(0),
    upload_(0),
    haltGui_(haltGui),
    sync_(sync)
{
    LOG;
    JsonReader jsonReader;
    repositories_ = jsonReader.repositories(sync);
    manageDeltaUpdates(jsonReader);

    LOG << "readJson completed";
    // TODO: Simply do not add torrents that are not in repositories.json
    if (sync_->ready())
    {
        removeOrphans();
    }
    else
    {
        connect(dynamic_cast<QObject*>(sync_), SIGNAL(initCompleted()), this, SLOT(removeOrphans()));
    }

    updateTimer.setInterval(1000);
    connect(&updateTimer, &QTimer::timeout, this, &TreeModel::update);
    updateTimer.start();
}

// Removes sync dirs which
// are not active in any repository in this program.
void TreeModel::removeOrphans()
{
    QSet<QString> keys;

    for (const Repository* repository : repositories_)
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
    sync_->cleanUnusedFiles(keys);
    LOG << "Done";
}

void TreeModel::manageDeltaUpdates(const JsonReader& jsonReader)
{
    const QString deltaUpdatesKey = jsonReader.deltaUpdatesKey();
    if (deltaUpdatesKey != QString())
    {
        sync_->setDeltaUpdatesFolder(deltaUpdatesKey);
        if (SettingsModel::deltaPatchingEnabled())
        {
            sync_->enableDeltaUpdates();
        }
    }
}

void TreeModel::setHaltGui(bool halt)
{
    haltGui_ = halt;
}

TreeModel::~TreeModel()
{
    LOG;
    DeletableDetector::printDeletables(repositories_);
}

void TreeModel::stopUpdates()
{
    for (Repository* repo : repositories_)
    {
        repo->stopUpdates();
    }
}

void TreeModel::moveFiles()
{
    for (Mod* mod : mods())
    {
        mod->moveFiles();
    }
}

void TreeModel::enableRepositories()
{
    for (Repository* repo : repositories_)
    {
        repo->startUpdates();
        repo->setTicked(true);
    }
}

// TODO: Delete, QML specific
void TreeModel::rowsChanged()
{
    LOG;
}

// TODO: Delete, QML specific
void TreeModel::checkboxClicked(const QModelIndex& index)
{
    SyncItem* item = static_cast<SyncItem*>(index.internalPointer());
    item->checkboxClicked();
}

QString TreeModel::bandwithString(int amount) const
{
    if (amount > 1000000)
    {
        double  downloadDouble = double(amount)/1000000;
        return QString::number(downloadDouble, 'g', 2) + " MB/s";
    }
    return QString::number(amount/1000) + " kB/s";
}

QSet<Mod*> TreeModel::mods() const
{
    // Filter duplicates
    QSet<Mod*> mods;
    for (Repository* repo : repositories_)
    {
        for (Mod* mod : repo->mods())
        {
            mods.insert(mod);
        }
    }
    return mods;
}

QString TreeModel::downloadStr() const
{
    return bandwithString(download_);
}

QString TreeModel::uploadStr() const
{
    return bandwithString(upload_);
}

//TODO: Remove, QML
void TreeModel::launch(const QModelIndex& repoIdx) const
{
    Repository* repo = static_cast<Repository*>(repoIdx.internalPointer());
    repo->start();
}

//TODO: Remove, QML
void TreeModel::join(const QModelIndex& repoIdx) const
{
    Repository* repo = static_cast<Repository*>(repoIdx.internalPointer());
    repo->join();
}

//TODO: Remove, QML
void TreeModel::check(const QModelIndex& idx)
{
    SyncItem* syncItem = static_cast<SyncItem*>(idx.internalPointer());
    if (syncItem)
    {
        LOG << "Rechecking: " << syncItem->name();
        syncItem->check();
    }
}

QString TreeModel::versionString() const
{
    return Constants::VERSION_STRING;
}

void TreeModel::updateSpeed()
{
    download_ = sync_->download();
    upload_ = sync_->upload();
}

bool TreeModel::ready(const QModelIndex& idx) const
{
    SyncItem* item = static_cast<SyncItem*>(idx.internalPointer());
    if (item)
    {
        return item->statusStr() == SyncStatus::READY;
    }

    LOG_ERROR << "Ticked asked from non-syncitem object: " << idx.internalPointer();
    return false;
}

void TreeModel::update()
{
    for (Repository* repository : repositories_)
    {
        repository->update();
    }
    updateSpeed();
}

QList<IRepository*> TreeModel::repositories() const
{
    QList<IRepository*> retVal;
    for (Repository* repo : repositories_)
    {
        retVal.append(repo);
    }
    return retVal;
}
