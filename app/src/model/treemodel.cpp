/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include <chrono>
#include <sys/types.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

#include "afisync.h"
#include "afisynclogger.h"
#include "apis/libtorrent/libtorrentapi.h"
#include "deletabledetector.h"
#include "destructionwaiter.h"
#include "global.h"
#include "mod.h"
#include "processmonitor.h"
#include "repository.h"
#include "settingsmodel.h"
#include "treemodel.h"

using namespace std::chrono_literals;

TreeModel::TreeModel(QObject* parent):
    QObject(parent),
    download_(0),
    upload_(0),
    sync_(nullptr)
{
    LOG;
    jsonReader_.readJson();
    createSync(jsonReader_.deltaUrls());
    jsonReader_.updateRepositoriesOffline(sync_, repositories_);

    QSet<QString> usedKeys;
    for (const auto& repo : repositories_)
    {
        for (const QString& key : repo->modKeys())
        {
            usedKeys.insert(key);
        }
    }
    sync_->cleanUnusedFiles(usedKeys);

    LOG << "repositories.json read successfully";

    updateTimer_.setInterval(1s);
    connect(&updateTimer_, &QTimer::timeout, this, &TreeModel::update);
    updateTimer_.start();

    repoUpdateTimer_.setInterval(30s);
    connect(&repoUpdateTimer_, &QTimer::timeout, this, &TreeModel::periodicRepoUpdate);
    repoUpdateTimer_.start();
}

void TreeModel::createSync(const QStringList& deltaUrls)
{
    if (deltaUrls.isEmpty())
    {
        sync_ = new LibTorrentApi();
    }
    else
    {
        sync_ = new LibTorrentApi(deltaUrls);
    }
    Global::sync = sync_;
}

TreeModel::~TreeModel()
{
    LOG;
    const DeletableDetector deletableDetector(SettingsModel::instance().modDownloadPath(), toIrepositories(repositories_));
    AfiSync::printDeletables(deletableDetector);

    updateTimer_.stop();
    repoUpdateTimer_.stop();

    QSet<QObject*> mods;
    for (const auto& repo : repositories_)
    {
        for (const auto& mod : repo->mods())
        {
            mods.insert(mod.get());
        }
    }
    DestructionWaiter waiter(mods);

    for (const auto& repo : repositories_)
    {
        repo->clearModAdapters();
    }
    repositories_.clear();
    waiter.wait();

    auto syncObject = dynamic_cast<QObject*>(sync_);
    DestructionWaiter syncWaiter(syncObject);
    syncObject->deleteLater();
    syncWaiter.wait(15); // libTorrent session delete might hang
}

void TreeModel::stopUpdates()
{
    for (const auto& repo : repositories_)
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
    for (const auto& repo : repositories_)
    {
        repo->startUpdates();
        repo->setTicked(true);
    }
}

QString TreeModel::bandwithString(int amount)
{
    if (amount > 1000000)
    {
        double  downloadDouble = double(amount)/1000000;
        return QString::number(downloadDouble, 'g', 2) + " MB/s";
    }
    return QString::number(amount/1000) + " kB/s";
}

const QSet<Mod*> TreeModel::mods() const
{
    // Filter duplicates
    QSet<Mod*> mods;
    for (const auto& repo : repositories_)
    {
        for (const auto& mod : repo->mods())
        {
            mods.insert(mod.get());
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
    for (const auto& repo : repositories_)
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
        mirroringDeltaPatches_ = false;
        LOG << "Updating repositories";
        updateRepositories();
        return;
    }
    LOG << "No repo updates";
}

void TreeModel::updateRepositories()
{
    const QSet<QString> removeFromSync = jsonReader_.getRemovables(repositories_);
    for (const QString& key : removeFromSync)
    {
        sync_->removeFolder(key);
    }
    jsonReader_.updateRepositories(sync_, repositories_);
    emit repositoriesChanged(toIrepositories(repositories_));
}

QList<IRepository*> TreeModel::toIrepositories(const QList<QSharedPointer<Repository>>& repositories)
{
    QList<IRepository*> irepositories;
    irepositories.reserve(repositories.size());
    for (const auto& repository : repositories)
    {
        irepositories.append(repository.get());
    }
    return irepositories;
}

void TreeModel::update()
{
    bool allDone = true;
    for (const auto& repository : repositories_)
    {
        repository->update();
        if (!repository->isReady())
        {
            allDone = false;
        }
    }
    updateSpeed();
    if (allDone && !mirroringDeltaPatches_ && Global::guiless)
    {
        LOG << "All mods downloaded. Mirroring delta patches ...";
        sync_->mirrorDeltaPatches();
        mirroringDeltaPatches_ = true;
    }
}

QList<IRepository*> TreeModel::repositories() const
{
    QList<IRepository*> retVal;
    retVal.reserve(repositories_.size());
    for (const auto& repo : repositories_)
    {
        retVal.append(repo.get());
    }
    return retVal;
}
