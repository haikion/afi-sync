/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include <csignal>
#include <sys/types.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStringList>
#include "afisync.h"
#include "afisynclogger.h"
#include "apis/libtorrent/libtorrentapi.h"
#include "deletabledetector.h"
#include "global.h"
#include "mod.h"
#include "modadapter.h"
#include "processmonitor.h"
#include "repository.h"
#include "settingsmodel.h"
#include "treeitem.h"
#include "treemodel.h"

TreeModel::TreeModel(QObject* parent):
    QObject(parent),
    download_(0),
    upload_(0),
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

TreeModel::~TreeModel()
{
    LOG;
    const DeletableDetector deletableDetector(SettingsModel::modDownloadPath(), toIrepositories(repositories_));
    AfiSync::printDeletables(deletableDetector);
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
                const QString key = mod->key();
                connect(mod, &QObject::destroyed, [=] (QObject* obj)
                {
                    Q_UNUSED(obj)
                    sync_->removeFolder(key);
                });
                mod->deleteLater();
            }
        }
        delete repo;
    }
    emit repositoriesChanged(toIrepositories(repositories_));
}

QList<IRepository*> TreeModel::toIrepositories(const QList<Repository*>& repositories)
{
    QList<IRepository*> irepositories;
    for (Repository* repository : repositories)
    {
        irepositories.append(repository);
    }
    return irepositories;
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
