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
#include "rootitem.h"
#include "global.h"
#include "modadapter.h"

const QHash<int, QByteArray> TreeModel::ROLE_NAMES({
                                   {TreeModel::Check, QByteArrayLiteral("check")},
                                   {TreeModel::Name, QByteArrayLiteral("name")},
                                   {TreeModel::Status, QByteArrayLiteral("status")},
                                   {TreeModel::Progress, QByteArrayLiteral("progress")},
                                   {TreeModel::Start, QByteArrayLiteral("start")},
                                   {TreeModel::Join, QByteArrayLiteral("join")},
                                   {TreeModel::FileSize, QByteArrayLiteral("fileSize")}
                               });

TreeModel::TreeModel(QObject* parent, bool haltGui):
    QObject(parent),
    //rootItem_(new RootItem(this)),
    download_(0),
    upload_(0),
    haltGui_(haltGui)
{
    LOG;
}

void TreeModel::setHaltGui(bool halt)
{
    haltGui_ = halt;
}

TreeModel::~TreeModel()
{
    LOG;
}

void TreeModel::reset()
{
}

void TreeModel::enableRepositories()
{
    LOG;
}

void TreeModel::rowsChanged()
{
    LOG;
}

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

QString TreeModel::downloadStr() const
{
    return bandwithString(download_);
}

QString TreeModel::uploadStr() const
{
    return bandwithString(upload_);
}

void TreeModel::launch(const QModelIndex& repoIdx) const
{
    Repository* repo = static_cast<Repository*>(repoIdx.internalPointer());
    repo->start();
}

void TreeModel::join(const QModelIndex& repoIdx) const
{
    Repository* repo = static_cast<Repository*>(repoIdx.internalPointer());
    repo->join();
}

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

void TreeModel::updateSpeed(qint64 download, qint64 upload)
{
    if (download != download_)
    {
        download_ = download;
        emit downloadChanged(downloadStr());
    }
    if (upload != upload_)
    {
        upload_ = upload;
        emit uploadChanged(uploadStr());
    }
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
