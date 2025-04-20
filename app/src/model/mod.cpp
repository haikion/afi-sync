#include "mod.h"

#include <chrono>

#include <QCoreApplication>
#include <QDirIterator>
#include <QStringLiteral>
#include <QTimer>
#include <QtMath>

#include "afisynclogger.h"
#include "apis/libtorrent/libtorrentapi.h"
#include "fileutils.h"
#include "global.h"
#include "installer.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

const unsigned Mod::COMPLETION_WAIT_DURATION = 0;

Mod::Mod(const QString& name, const QString& key, LibTorrentApi* libTorrentApi):
    SyncItem(name),
    libTorrentApi_(libTorrentApi),
    key_(key)
{
    setStatus(SyncStatus::STARTING);
    //Enables non lagging UI
    moveToThread(Global::workerThread);
    QMetaObject::invokeMethod(this, &Mod::threadConstructor, Qt::QueuedConnection);
}

Mod::~Mod()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    LOG << name() << " destructed";
}

void Mod::threadConstructor()
{
    updateTicked();
    updateTimer_ = new QTimer(this);
    updateTimer_->setTimerType(Qt::VeryCoarseTimer);
    updateTimer_->setInterval(1s);

    connect(updateTimer_, &QTimer::timeout, this, &Mod::update);

    connect(libTorrentApi_, &LibTorrentApi::initCompleted, this, &Mod::repositoryChanged);
    connect(libTorrentApi_, &LibTorrentApi::folderAdded, this, &Mod::onFolderAdded);
    LOG << name() << " created";
}

QString Mod::path()
{
    return settings_.modDownloadPath() + "/" + name();
}

void Mod::init()
{
    updateTicked();
    repositoryChanged();
}

void Mod::update()
{
    updateStatus();
    updateProgress();
}

//Removes mod which has same name but different key.
void Mod::removeConflicting()
{
    for (const QString& syncKey : libTorrentApi_->folderKeys())
    {
        QString syncPath = libTorrentApi_->folderPath(syncKey);

        if (path().toLower() == syncPath.toLower() && syncKey != key_)
        {
            //Downloading into same folder but the key is different!
            LOG << "Removing conflicting (" << syncKey << ", " << syncPath << ") for (" << key_ << ", " << path() << ").";
            libTorrentApi_->removeFolder(syncKey);
            return;
        }
    }
}

//Starts sync directory. If directory does not exist
//folder will be created. If path is incorrect
//folder will be re-created correctly.
//If mod is not in sync it will be added to it.
void Mod::start()
{
    if (!libTorrentApi_->folderExists(key_))
    {
        removeConflicting();
        //Add folder
        bool resumeDataFound = libTorrentApi_->addFolder(key_, name());
        setProcessCompletion(!resumeDataFound);
    }
}

void Mod::onFolderAdded(const QString &key)
{
    if (key != key_)
    {
        return; // Not for me
    }
    if (moveFilesPostponed_)
    {
        moveFilesPostponed_ = false;
        moveFilesNow(moveFilesPostponedOverwrite_);
        moveFilesPostponedOverwrite_ = true; // Set to default
        LOG << "Files moved to " << settings_.modDownloadPath();
    }
    if (fileSize() == 0)
    {
        auto size = libTorrentApi_->folderFileSize(key_);
        setFileSize(size);
        emit fileSizeInitialized(size);
        LOG << name() << " size set to " << size;
    }

    //Do the actual starting
    libTorrentApi_->setFolderPaused(key_, false);
    update();
    startUpdates();
}

void Mod::moveFiles(bool overwrite)
{
    QMetaObject::invokeMethod(this, [=, this] () {
        if (statusStr() == SyncStatus::PATCHING ||
            statusStr() == SyncStatus::DOWNLOADING_PATCHES ||
            statusStr() == SyncStatus::EXTRACTING_PATCH ||
            statusStr() == SyncStatus::STARTING)
        {
            LOG << "Waiting for mod patching to finish before moving files ...";
            moveFilesPostponed_ = true;
            moveFilesPostponedOverwrite_ = overwrite;
            return;
        }
        moveFilesNow(overwrite);
        LOG << name() <<  " moved to " << settings_.modDownloadPath();
    }, Qt::QueuedConnection);
}

void Mod::moveFilesNow(bool overwrite)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (libTorrentApi_->folderExists(key_))
    {
        deleteExtraFiles();
        libTorrentApi_->setFolderPath(key_, settings_.modDownloadPath(), overwrite);
    } else {
        libTorrentApi_->removeTorrentParams(key_);
    }
}

void Mod::setProcessCompletion(const bool value)
{
    settings_.setProcessed(name(), !value ? key_ : ""_L1);
}

bool Mod::getProcessCompletion()
{
    return settings_.processed(name()) != key_;
}

qint64 Mod::totalWanted() const
{
    return totalWanted_;
}

qint64 Mod::totalWantedDone() const
{
    return totalWantedDone_;
}

void Mod::updateProgress()
{
    if (!active() || !libTorrentApi_->folderExists(key_))
    {
        totalWanted_ =  -1;
        totalWantedDone_ = -1;
        return;
    }

    totalWanted_ = libTorrentApi_->folderTotalWanted(key_);
    totalWantedDone_ = libTorrentApi_->folderTotalWantedDone(key_);
}

bool Mod::stop()
{
    //TODO: Remove? Might be too defensive
    if (!libTorrentApi_->folderExists(key_))
    {
        LOG_ERROR << "Folder" << name() << "does not exist.";
        return false;
    }

    libTorrentApi_->setFolderPaused(key_, true);
    stopUpdatesSlot();
    update();

    LOG << "Torrent paused for " << name();
    return true;
}

void Mod::deleteExtraFiles()
{
    using std::as_const;

    if (!ticked())
    {
        LOG << name()  << " is inactive, extra file deletion skipped.";
        return;
    }

    QSet<QString> localFiles;
    QSet<QString> remoteFiles = libTorrentApi_->folderFilesUpper(key_);

    QDir dir(settings_.modDownloadPath() + "/" + name());
    QDirIterator it(dir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        localFiles.insert(it.next().toUpper());
    }
    //Fail safe if there is torrent without files.
    if (remoteFiles.isEmpty())
    {
        LOG_WARNING << "Not deleting extra files because torrent contains 0 files.";
        return; //Would delete everything otherwise
    }

    QSet<QString> extraFiles = localFiles - remoteFiles;
    for (const QString& path : as_const(extraFiles))
    {
        FileUtils::rmCi(path);
    }
    FileUtils::safeRemoveEmptyDirs(dir.absolutePath());
}

bool Mod::ticked()
{
    return ticked_;
}

void Mod::updateTicked()
{
    if (!optional() && !reposInactive())
    {
        ticked_ = true;
        return;
    }

    for (ModAdapter* adp : modAdapters())
    {
        if (adp->ticked() && adp->repo()->ticked())
        {
            ticked_ = true;
            return;
        }
    }
    ticked_ = false;
}

//If all repositories this mod is included in are disabled then stop the
//download.
void Mod::repositoryChanged()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    if (!libTorrentApi_->ready())
    {
        return;
    }
    updateTicked();
    if (reposInactive() || !ticked())
    {
        // No checking for paused state because torrent might have been
        // paused by auto management (queued for example).
        if (libTorrentApi_->folderExists(key_))
        {
            LOG << "All repositories inactive or mod unchecked. Stopping " << name();
            stop();
        }
        return;
    }
    //At least one repo active and mod checked
    if (!libTorrentApi_->folderExists(key_) || libTorrentApi_->folderPaused(key_))
    {
        start();
    }
    updateTicked();
}

void Mod::removeModAdapter(ModAdapter* modAdapter)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    adapters_.removeOne(modAdapter);
    Q_ASSERT(!adapters_.contains(modAdapter));

    repositoryChanged();
}

const QSet<Repository*> Mod::repositories() const
{
    QSet<Repository*> retVal;
    for (ModAdapter* modAdapter : adapters_)
    {
        retVal.insert(modAdapter->repo());
    }
    return retVal;
}

QString Mod::key() const
{
    return key_;
}

void Mod::startUpdates()
{
    QMetaObject::invokeMethod(this, &Mod::startUpdatesSlot, Qt::QueuedConnection);
}

void Mod::startUpdatesSlot()
{
    if (libTorrentApi_->ready())
    {
        updateTimer_->start();
    }
    else
    {
        connect(libTorrentApi_, &LibTorrentApi::initCompleted, updateTimer_, QOverload<>::of(&QTimer::start));
    }
}

//This should always be run in UI Thread
void Mod::stopUpdates()
{
    //Updates need to be stopped before object destruction, hence blocking.
    QMetaObject::invokeMethod(this, &Mod::stopUpdatesSlot, Qt::BlockingQueuedConnection);
}

void Mod::stopUpdatesSlot()
{
    updateTimer_->stop();
}

bool Mod::selected()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    for (ModAdapter* adp : modAdapters())
    {
        if (adp->selected())
        {
            return true;
        }
    }
    return false;
}

//Returns true only if at least one adapter is optional
bool Mod::optional()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    for (ModAdapter* adp : modAdapters())
    {
        if (adp->optional()) {
            return true;
        }
    }
    return false;
}

QList<ModAdapter*> Mod::modAdapters()
{
    adaptersMutex_.lock();
    QList<ModAdapter*> retVal = adapters_;
    adaptersMutex_.unlock();
    return retVal;
}

void Mod::appendModAdapter(ModAdapter* adapter)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    adaptersMutex_.lock();
    adapters_.append(adapter);
    auto size = adapters_.size();
    adaptersMutex_.unlock();

    if (size == 1)
    {
        //First repository added -> initialize mod.
        if (libTorrentApi_->ready())
        {
            QMetaObject::invokeMethod(this, &Mod::init, Qt::QueuedConnection);
        }
        else
        {
            connect(libTorrentApi_, &LibTorrentApi::initCompleted, this, &Mod::init);
        }
        return;
    }
    QMetaObject::invokeMethod(this, &Mod::repositoryChanged, Qt::QueuedConnection);
}

void Mod::updateStatus()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    if (!ticked() || reposInactive())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if (!libTorrentApi_->folderExists(key_))
    {
        setStatus(SyncStatus::ERRORED + u" Failed to load torrent"_s);
    }
    else if (libTorrentApi_->folderMovingFiles(key_))
    {
        setStatus(SyncStatus::MOVING_FILES);
    }
    else if (libTorrentApi_->folderQueued(key_))
    {
        setStatus(SyncStatus::QUEUED);
    }
    else if (libTorrentApi_->folderCheckingPatches(key_))
    {
        setStatus(SyncStatus::CHECKING_PATCHES);
    }
    else if (libTorrentApi_->folderChecking(key_))
    {
        setStatus(SyncStatus::CHECKING);
    }
    else if (libTorrentApi_->folderDownloadingPatches(key_))
    {
        setStatus(SyncStatus::DOWNLOADING_PATCHES);
    }
    else if (libTorrentApi_->folderDownloading(key_))
    {
        if (statusStr() != SyncStatus::DOWNLOADING)
        {
            setStatus(SyncStatus::DOWNLOADING);
            //Heavy operation -> only set when entering downloading state.
            setProcessCompletion(true);
        }
    }
    //Wait for ready just in case
    else if (statusStr() == SyncStatus::WAITING)
    {
        ++waitTime_;
        if (waitTime_ > COMPLETION_WAIT_DURATION)
        {
            waitTime_ = 0;
            if (getProcessCompletion())
            {
                processCompletion();
                setProcessCompletion(false);
            }
            setStatus(SyncStatus::READY);
        }
    }
    else if (statusStr() == SyncStatus::READY || statusStr() == SyncStatus::READY_PAUSED) {}
    else if (libTorrentApi_->folderReady(key_))
    {
        setStatus(SyncStatus::WAITING);
    }
    else
    {
        //New block for error initialization
        const QString error = libTorrentApi_->folderError(key_);
        if (!error.isEmpty())
        {
            setStatus(SyncStatus::ERRORED + error);
        }
        else if (libTorrentApi_->folderPaused(key_))
        {
            if (statusStr() == SyncStatus::READY)
            {
                setStatus(SyncStatus::READY_PAUSED);
            }
            else
            {
                setStatus(SyncStatus::PAUSED);
            }
        }
        else if (libTorrentApi_->folderExtractingPatch(key_))
        {
            setStatus(SyncStatus::EXTRACTING_PATCH);
        }
        else if (libTorrentApi_->folderPatching(key_))
        {
            setStatus(SyncStatus::PATCHING);
        }
        else if (libTorrentApi_->folderNoPeers(key_))
        {
            setStatus(SyncStatus::NO_PEERS);
        }
        else
        {
            LOG_ERROR << "Unable to determine mod state. name = " << name();
            setStatus(SyncStatus::ERRORED);
        }
    }

}

void Mod::forceCheck()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    libTorrentApi_->disableQueue(key_);
    check();
}

// Thread safe
QString Mod::progressStr()
{
    if (!ticked() || statusStr() == SyncStatus::STARTING)
    {
        return u"???"_s;
    }

    return toProgressStr(totalWanted_, totalWantedDone_);
}

QString Mod::bytesToMegasCeilStr(const qint64 bytes)
{
    if (bytes == 0)
    {
        return QString::number(0);
    }

    return QString::number(1 + ((bytes - 1) / Constants::MEGA_DIVIDER));  // Ceil division
}

QString Mod::toProgressStr(const qint64 totalWanted, qint64 totalWantedDone)
{
    if (totalWanted < 0 || totalWantedDone < 0)
    {
        return u"???"_s;
    }

    // Do not ceil to 100 %
    if (totalWantedDone < totalWanted)
    {
        totalWantedDone = totalWantedDone > Constants::MEGA_DIVIDER ? totalWantedDone - Constants::MEGA_DIVIDER : 0;
    }
    return u"%1 / %2"_s.arg(bytesToMegasCeilStr(totalWantedDone),
                                         bytesToMegasCeilStr(totalWanted));
}

void Mod::processCompletion()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    libTorrentApi_->truncateOvergrownFiles(key_);
    deleteExtraFiles();
    if (!Global::isMirror)
    {
        Installer::install(this);
    }
    LOG <<  name() << " synced";
}

void Mod::check()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

    // User can start check manually twice
    if (!libTorrentApi_->folderChecking(key_)
        && libTorrentApi_->checkFolder(key_))
    {
        // Process completion when checking is done
        setProcessCompletion(true);
    }
}

void Mod::checkboxClicked()
{
    QMetaObject::invokeMethod(this, &Mod::checkboxClickedSlot, Qt::QueuedConnection);
}

void Mod::checkboxClickedSlot()
{
    updateTicked();
    //Below cmd will start the download if repository is active.
    repositoryChanged();
}

bool Mod::reposInactive() const
{
    for (Repository* repo : repositories())
    {
        if (repo->statusStr() != SyncStatus::INACTIVE)
        {
            return false;
        }
    }
    return true;
}
