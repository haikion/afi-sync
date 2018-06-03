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
    QAbstractItemModel(parent),
    rootItem_(new RootItem(this)),
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
    delete rootItem_;
}

void TreeModel::reset()
{
    rootItem_->resetSyncSettings();
}

void TreeModel::enableRepositories()
{
    LOG;
    rootItem_->enableRepositories();
}

void TreeModel::rowsChanged()
{
    LOG;
    emit layoutChanged();
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
        emit downloadChanged(getDownload());
    }
    if (upload != upload_)
    {
        upload_ = upload;
        emit uploadChanged(getUpload());
    }
}

bool TreeModel::ready(const QModelIndex& idx) const
{
    SyncItem* item = static_cast<SyncItem*>(idx.internalPointer());
    if (item)
    {
        return item->status() == SyncStatus::READY;
    }

    LOG_ERROR << "ticked asked from non-syncitem object:" << idx.internalPointer();
    return false;
}

//For testability
RootItem* TreeModel::rootItem() const
{
    return rootItem_;
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
    //Only update dynamic columns
    static const QVector<int> roles({Check, Progress, Status, Start, Join});

    if (haltGui_)
    {
        LOG << "UI updates halted.";
        return;
    }

    if (row == -1)
        row = item->row();

    QModelIndex idx = createIndex(row, 0, item);
    if (!idx.isValid())
    {
        LOG_ERROR << "Tried to update invalid index.";
        return;
    }

    emit dataChanged(idx, idx, roles);
}

QVariant TreeModel::data(const QModelIndex& index, int role = Qt::DisplayRole) const
{
   SyncItem* item = static_cast<SyncItem*>(index.internalPointer());

   if (!index.isValid() && item == nullptr)
        return QVariant();

    switch (role)
    {
        case Qt::DisplayRole: return item->data(index.column());
        case Check: return item->checkText();
        case Name: return item->nameText();
        case Status: return item->statusText();
        case Progress: return item->progressText();
        case Start: return item->startText();
        case Join: return item->joinText();
        case FileSize: return item->fileSizeText();
    }
    Q_ASSERT_X(false, "TreeModel::data","Incorrect role = " + QString::number(role).toLatin1());
    return QVariant();
}


Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const
{
    if (haltGui_ || !index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QHash<int, QByteArray> TreeModel::roleNames() const
{
    return ROLE_NAMES;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent)
            const
{
    if (haltGui_ || !hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem* parentItem;

    if (!parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    if(!parentItem)
        return QModelIndex();

    TreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex& index) const
{
    if (haltGui_ || !index.isValid())
        return QModelIndex();

    TreeItem* childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem* parentItem = childItem->parentItem();

    if (parentItem == rootItem_ || parentItem == 0)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const
{
    TreeItem* parentItem;
    if (parent.column() > 0)
        return 0;

    if (haltGui_ || !parent.isValid())
        parentItem = rootItem_;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}
