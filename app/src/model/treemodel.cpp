/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include <sys/types.h>
#include <signal.h>
#include "apis/libtorrent/libtorrentapi.h"
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
#include "processmonitor.h"

TreeModel::TreeModel(QObject* parent):
    QObject(parent),
    download_(0),
    upload_(0),
    haltGui_(false), //TODO: Remove, QML
    sync_(nullptr)
{
    LOG;
    createSync(jsonReader_);
    repositories_ = jsonReader_.repositories(sync_);
    LOG << "readJson completed";

    updateTimer_.setInterval(1000);
    connect(&updateTimer_, &QTimer::timeout, this, &TreeModel::update);
    updateTimer_.start();

    repoUpdateTimer_.setInterval(30000);
    connect(&repoUpdateTimer_, &QTimer::timeout, this, &TreeModel::periodicRepoUpdate);
    repoUpdateTimer_.start();

    qRegisterMetaType<Repository*>("Repository*");
}

//TODO: Remove
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

void TreeModel::createSync(const JsonReader& jsonReader)
{
    const QString deltaUpdatesKey = jsonReader.deltaUpdatesKey();
    if (deltaUpdatesKey != QString())
    {
        Global::sync = new LibTorrentApi(deltaUpdatesKey);
        sync_ = Global::sync;
        return;
    }
    Global::sync = new LibTorrentApi();
    sync_ = Global::sync;
}

void TreeModel::setHaltGui(bool halt)
{
    haltGui_ = halt;
}

TreeModel::~TreeModel()
{
    LOG;
    // FIXME: Move to main.cpp
    //DeletableDetector::printDeletables(repositories_);
    delete sync_;
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

void TreeModel::periodicRepoUpdate()
{
    for (Repository* repo : repositories_)
    {
        const QString status = repo->statusStr();
        if (status != SyncStatus::READY &&
                status != SyncStatus::READY_PAUSED && status != SyncStatus::INACTIVE)
        {
            LOG << "Periodic update disabled, " << repo->name() << " " << status;
            return;
        }
    }
    if (ProcessMonitor::arma3Running())
    {
        LOG << "Repositories update ignored because Arma 3 is running";
        return;
    }
    if (jsonReader_.updateAvailable())
    {
        LOG << "Updating repositories";
        updateRepositories();
        return;
    }
    LOG << "No repo updates";
}

void TreeModel::updateRepositories()
{
    QList<Repository*> updatedList = repositories_;
    jsonReader_.updateRepositories(sync_, updatedList);

    QList<Repository*> deletables;
    for (Repository* repo : repositories_)
    {
        if (!updatedList.contains(repo))
        {
            deletables.append(repo);
        }
    }

    repositories_.clear();
    repositories_.append(updatedList);

    for (Repository* repo : deletables)
    {
        QList<Mod*> mods = repo->mods();
        repo->clearModAdapters();
        for (Mod* mod : mods)
        {
            mod->removeRepository(repo);
            if (mod->repositories().isEmpty())
            {
                mod->deleteLater();
            }
        }
        delete repo;
    }
    emit repositoriesChanged(toIrepositories(repositories_));
}

QList<IRepository*> TreeModel::toIrepositories(const QList<Repository*> repositories)
{
    QList<IRepository*> irepositories;
    for (Repository* repository : repositories)
    {
        irepositories.append(repository);
    }
    return irepositories;
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
