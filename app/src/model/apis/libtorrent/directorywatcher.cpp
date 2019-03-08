#include <QThread>
#include "../../fileutils.h"
#include "../../global.h"
#include "directorywatcher.h"

DirectoryWatcher::DirectoryWatcher(const QString& fromPath, const QString& toPath, const QString& key, const qint64& totalWanted):
    fromPath_(fromPath),
    toPath_(toPath),
    key_(key),
    totalWanted_(totalWanted)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    totalWanted_ = FileUtils::dirSize(fromPath);
}

bool DirectoryWatcher::done()
{
    totalWantedDone_=  FileUtils::dirSize(toPath_);
    return totalWantedDone_ == totalWanted_;
}

QString DirectoryWatcher::key() const
{
    return key_;
}

qint64 DirectoryWatcher::totalWanted() const
{
    return totalWanted_;
}

qint64 DirectoryWatcher::totalWantedDone() const
{
    return totalWantedDone_;
}
