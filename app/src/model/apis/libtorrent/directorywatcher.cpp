#include <QThread>
#include "../../fileutils.h"
#include "../../global.h"
#include "directorywatcher.h"

DirectoryWatcher::DirectoryWatcher(const QString& fromPath, const QString& toPath, const QString& key, const qint64 totalWantedForce):
    fromPath_(fromPath),
    toPath_(toPath),
    totalWanted_(totalWantedForce),
    key_(key)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (totalWanted_ < 0) {
        totalWanted_ = FileUtils::dirSize(fromPath);
    }
}

bool DirectoryWatcher::done()
{
    totalWantedDone_=  FileUtils::dirSize(toPath_);
    // Libtorrent will delete the source directory (fromPath_) once move_storage is done
    return totalWantedDone_ == totalWanted_ || !QFileInfo::exists(fromPath_);;
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
