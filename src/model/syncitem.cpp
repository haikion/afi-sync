#include <limits>
#include <QTime>
#include "afisynclogger.h"
#include "global.h"
#include "syncitem.h"

SyncItem::SyncItem(const QString& name, TreeItem* parentItem):
    TreeItem(name, parentItem),
    name_(name),
    status_(SyncStatus::NO_SYNC_CONNECTION),
    eta_(Constants::MAX_ETA),
    fileSize_(0)
{
}

QString SyncItem::checkText()
{
    QString rVal = ticked() ? "true" : "false";
    return rVal;
}

QString SyncItem::nameText()
{
    return name_;
}

QString SyncItem::statusStr()
{
    return status_;
}

QString SyncItem::sizeStr() const
{
    if (fileSize_ == 0)
        return QString("??.?? MB");

    double size = fileSize_;
    int i = 0;
    QStringList list;
    list << "B" << "kB" << "MB" << "GB";

    for (i = 0; i < list.size() && size > 1024; ++i)
        size = size / 1024;

    return QString::number(size, 'f', 2) + " " + list.at(i);
}

bool SyncItem::active() const
{
    static QSet<QString> activeStatuses = createActiveStatuses();
    return activeStatuses.contains(status_);
}

QString SyncItem::startText()
{
    return "uploadText()"; //TODO: Wtf??
}

int SyncItem::eta() const
{
    return eta_;
}

QString SyncItem::etaStr() const
{
    //In repository multiple MAX_ETA maybe summed so we use >=.
    if (eta_ >= Constants::MAX_ETA)
        return "??:??:??";

    return QTime(0,0,0).addSecs(eta_).toString();
}

void SyncItem::setEta(const int& eta)
{
    eta_ = eta;
}

QSet<QString> SyncItem::createActiveStatuses()
{
    QSet<QString> retVal;
    retVal.insert(SyncStatus::DOWNLOADING);
    retVal.insert(SyncStatus::CHECKING);
    retVal.insert(SyncStatus::DOWNLOADING_PATCHES);
    retVal.insert(SyncStatus::NO_PEERS);
    retVal.insert(SyncStatus::PATCHING);
    retVal.insert(SyncStatus::QUEUED);
    retVal.insert(SyncStatus::READY);
    retVal.insert(SyncStatus::READY_PAUSED);
    return retVal;
}

QString SyncItem::name() const
{
    return name_;
}

void SyncItem::setName(const QString& name)
{
    name_ = name;
}

quint64 SyncItem::fileSize() const
{
    return fileSize_;
}

void SyncItem::setFileSize(const quint64 size)
{
    fileSize_ = size;
}

void SyncItem::setStatus(const QString& status)
{
    status_ = status;
}

QString SyncItem::statusStr() const
{
    return status_;
}
