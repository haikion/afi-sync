#include <QStringLiteral>
#include <QTime>
#include "syncitem.h"

using namespace Qt::StringLiterals;

SyncItem::SyncItem(const QString& name):
    name_(name),
    fileSize_(0)
{
    setStatus(SyncStatus::NO_SYNC_CONNECTION);
}

// Creates a copy of status due to concurrency
QString SyncItem::statusStr()
{
    QString retVal;
    statusMutex_.lock();
    retVal = status_;
    statusMutex_.unlock();
    return retVal;
}

QString SyncItem::sizeStr() const
{
    if (fileSize_ == 0)
    {
        return u"??.?? MB"_s;
    }

    double size = fileSize_;
    int i = 0;
    QStringList list;
    list << u"B"_s << u"kB"_s << u"MB"_s
         << u"GB"_s;

    for (i = 0; i < list.size() && size > 1024; ++i)
    {
        size = size / 1024;
    }

    return QString::number(size, 'f', 2) + " " + list.at(i);
}

bool SyncItem::active() const
{
    static QSet<QString> activeStatuses = createActiveStatuses();
    return activeStatuses.contains(status_);
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
    retVal.insert(SyncStatus::CHECKING_PATCHES);
    retVal.insert(SyncStatus::MOVING_FILES);
    return retVal;
}

QString SyncItem::name() const
{
    return name_;
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
    statusMutex_.lock();
    if (status_ != status)
    {
        status_ = status;
    }
    statusMutex_.unlock();
}
