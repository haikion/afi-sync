/*
    treemodel.cpp

    Provides a simple tree model to show how to create and use hierarchical
    models.
*/

#include <sys/types.h>
#include <signal.h>
#include "debug.h"
#include <QDir>
#include <QStringList>
#include <QThread>
#include <QFile>
#include <QCoreApplication>
#include "apis/btsync/libbtsync-qt/bts_global.h"
#include "treeitem.h"
#include "repository.h"
#include "mod.h"
#include "treemodel.h"
#include "rootitem.h"
#include "global.h"

TreeModel::TreeModel(const QString& username, const QString& password,
                     unsigned port, QObject* parent) :
    QAbstractItemModel(parent),
    rootItem_(new RootItem(username, password, port, this)),
    download_(0),
    upload_(0)
{
    DBG;
}

TreeModel::TreeModel(QObject* parent) :
    QAbstractItemModel(parent),
    rootItem_(new RootItem(this)),
    download_(0),
    upload_(0)
{
    DBG << "done";
}

TreeModel::~TreeModel()
{
    DBG;
    delete rootItem_;
}

void TreeModel::reset()
{
    rootItem_->resetSyncSettings();
}

void TreeModel::enableRepositories()
{
    DBG;
    rootItem_->enableRepositories();
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

QString TreeModel::getDownload() const
{
    return bandwithString(download_);
}

QString TreeModel::getUpload() const
{
    return bandwithString(upload_);
}

void TreeModel::launch(const QModelIndex& repoIdx) const
{
    Repository* repo = static_cast<Repository*>(repoIdx.internalPointer());
    repo->launch();
}

void TreeModel::join(const QModelIndex& repoIdx) const
{
    Repository* repo = static_cast<Repository*>(repoIdx.internalPointer());
    repo->join();
}

void TreeModel::resetSync()
{
    rootItem_->resetSyncSettings();
}

void TreeModel::processCompletion()
{
    rootItem_->processCompletion();
}

void TreeModel::updateSpeed(qint64 download, qint64 upload)
{
    //DBG << "download= " << download << " upload =" << upload;
    if (download != download_)
    {
        download_ = download;
        emit downloadChanged(getDownload());
    }
    if (upload != upload_)
    {
        upload_ = upload;
        emit uploadChanged(getUpload());
    }
}

bool TreeModel::isRepository(const QModelIndex& index) const
{
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    if (!item || !item->parentItem())
    {
        return false;
    }
    return item->parentItem() == rootItem_;
}


int TreeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem_->columnCount();
}

void TreeModel::updateView(TreeItem* item, int row)
{
    if (row == -1)
    {
        row = item->row();
    }
    QModelIndex idx = createIndex(row, 0, item);
    emit dataChanged(idx, idx);
}

QVariant TreeModel::data(const QModelIndex &index, int role = Qt::DisplayRole) const
{
   //DBG << "role = " << role;
   SyncItem* item = static_cast<SyncItem*>(index.internalPointer());

   if (!index.isValid() && item == nullptr)
        return QVariant();

    switch (role) {
        case Check: return item->checkText();
        case Name: return item->nameText();
        case Status: return item->statusText();
        case Progress: return item->progressText();
        case Start: return item->startText();
        case Join: return item->joinText();
    }
    Q_ASSERT(false);
    return QVariant();
}


Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    //DBG;

    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QHash<int,QByteArray> TreeModel::roleNames() const
{
    //DBG;

    QHash<int, QByteArray> result;
    result.insert(Check, QByteArrayLiteral("check"));
    result.insert(Name, QByteArrayLiteral("name"));
    result.insert(Status, QByteArrayLiteral("status"));
    result.insert(Progress, QByteArrayLiteral("progress"));
    result.insert(Start, QByteArrayLiteral("start"));
    result.insert(Join, QByteArrayLiteral("join"));
    return result;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent)
            const
{
    DBG << "row =" << row << "column =" << column;

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    if(!parentItem)
    {
        DBG << "Invalid request. row=" << row
                 << "column =" << column << "parent: " << parent;
        return QModelIndex();
    }
    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem_ || parentItem == 0)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
    TreeItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}
