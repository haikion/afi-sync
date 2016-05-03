#include "debug.h"
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QTime> //performance testing
#include <QEventLoop>
#include <QDirIterator>
#include "global.h"
#include "settingsmodel.h"
#include "mod.h"
#include "repository.h"
#include "installer.h"

Mod::Mod(const QString& name, const QString& key, bool isOptional):
    SyncItem(name, 0),
    isOptional_(isOptional),
    key_(key),
    sync_(0)
{
    DBG;
    setStatus(SyncStatus::NO_BTSYNC_CONNECTION);
    moveToThread(Global::workerThread);
    updateTimer_.moveToThread(Global::workerThread);
    qRegisterMetaType<QVector<int>>("QVector<int>");
}

Mod::~Mod()
{
    DBG;
    QMetaObject::invokeMethod(this, "threadDestructor", Qt::BlockingQueuedConnection);
}


void Mod::threadDestructor()
{
    DBG;
    updateTimer_.stop();
}

void Mod::init()
{
    DBG << " current thread: " << QThread::currentThread() << " Worker thread: " << Global::workerThread;

    if (!isOptional_)
    {
        setChecked(true);
    }
    repositoryEnableChanged();
    //Periodically check mod progress
    updateTimer_.setInterval(1000);
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(fetchEta()));
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateTimer_.start();
    DBG << "Completed name =" << name();
}

//Starts sync directory. If directory does not exist
//directory will be created. If path is incorrect
//directory will be re-created correctly.
void Mod::start()
{
    DBG << "name =" << name();
    QString modPath = SettingsModel::modDownloadPath();
    QString dir = QDir::toNativeSeparators(modPath + "/" + name());
    QString syncPath = QDir::toNativeSeparators(sync_->getFolderPath(key_));
    QString error = sync_->error(key_);

    if (syncPath.toUpper() != dir.toUpper() || error != "")
    {
        DBG << "Re-adding directory. syncPath ="
            << syncPath << "dir =" << dir
            << "error =" << error;
        //Disagreement between Sync and AfiSync
        sync_->removeFolder2(key_);
        sync_->addFolder(dir, key_, true);
    }
    QVariantMap response = sync_->setFolderPaused(key_, false);
    DBG << "response =" << response << "name =" << name();
}

void Mod::deleteExtraFiles()
{
    DBG;

    QSet<QString> localFiles;
    QSet<QString> remoteFiles = sync_->getFilesUpper(key_);

    QDir dir(SettingsModel::modDownloadPath() + "/" + name());
    QDirIterator it(dir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        localFiles.insert(it.next().toUpper());
    }
    //In case of incorrect configuration... or no peers (thx bts)
    //or ..bts sends incorrect files list...
    if (remoteFiles.size() == 0 || status() != SyncStatus::READY)
    {
        DBG << "Warning: Not deleting extra files because mod is not fully synced. =" << name();
        return; //Would delete everything otherwise
    }

    DBG << " name =" << name()
             << "\n\n remoteFiles =" << remoteFiles
             << "\n\n localFiles =" << localFiles;

    QSet<QString> extraFiles = localFiles - remoteFiles;
    for (QString file : extraFiles)
    {
        DBG << "Deleting extra file: " << file << " from mod =" << name();
        QFile(file).remove();
    }
}

bool Mod::checked() const
{
    if (!isOptional_)
    {
        return true;
    }
    return SyncItem::checked();
}

QString Mod::checkText()
{
    if (!isOptional_)
    {
        return "disabled";
    }
    return SyncItem::checkText();
}

QString Mod::startText()
{
    return "hidden";
}

QString Mod::joinText()
{
    return "hidden";
}

//If all repositories this mod is included in are disabled then stop the
//download.
void Mod::repositoryEnableChanged(bool offline)
{
    DBG << " current thread: " << QThread::currentThread() << " Worker thread: " << Global::workerThread;
    //just in case
    if (!sync_)
    {
        return;
    }
    bool allDisabled = true;
    for (const Repository* repo : repositories_)
    {
        DBG <<"checking mod name=" << name() << " repo name=" << repo->name();
        if (repo->checked())
        {
            //At least one repo is active
            allDisabled = false;
            break;
        }
    }
    if (offline)
    {
        DBG << "Offline, not updating Sync";
        return;
    }
    if (allDisabled || !checked())
    {
        //All repositories unchecked
        DBG << "Stopping mod sync dir name =" << name();
        sync_->setFolderPaused(key_, true);
        return;
    }
    //At least one repo active and mod checked
    DBG << "Starting mod: " << name();
    start();
}

std::vector<Repository*> Mod::repositories() const
{
    return repositories_;
}

int Mod::lastModified()
{
    return sync_->getLastModified(key_);
}

QString Mod::key() const
{
    return key_;
}

void Mod::addRepository(Repository* repository)
{
    DBG;

    repositories_.push_back(repository);
    if (repositories().size() == 1)
    {
        Repository* repo = repositories_.at(0);
        sync_ = repo->btsync();
        if (sync_->ready())
        {
            DBG << "Calling init directly";
            QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
        }
        else
        {
            connect(sync_, SIGNAL(initCompleted()), this, SLOT(init()));
            DBG << "initCompleted connection created";
        }
    }
}

void Mod::removeRepository(Repository* repository)
{
    DBG;

    //disconnect(repository, SIGNAL(enableChanged()), this, SLOT(repositoryEnableChanged()));
    auto it = std::find(repositories_.begin(), repositories_.end(), repository);
    if (it == repositories_.end())
    {
        DBG << "ERROR: Mod" << name() << "not found in repository:" << repository->name();
        return;
    }
    repositories_.erase(it);
}

void Mod::updateStatus()
{
    if (!checked())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if (!sync_->exists(key_))
    {
        setStatus(SyncStatus::NOT_IN_BTSYNC);
    }
    else if (eta() > 0)
    {
        setStatus(SyncStatus::DOWNLOADING);
    }
    else if (sync_->isIndexing(key_))
    {
        setStatus(SyncStatus::INDEXING);
    }
    else if (sync_->noPeers(key_))
    {
        setStatus(SyncStatus::NO_PEERS);
    }
    else if (sync_->paused(key_))
    {
        setStatus(SyncStatus::PAUSED);
    }
    else if (sync_->getSyncLevel(key_) == SyncLevel::SYNCED)
    {
        setStatus(SyncStatus::READY);
    }
}

void Mod::checkboxClicked()
{
    SyncItem::checkboxClicked();
    DBG << "checked()=" << checked();
    QMetaObject::invokeMethod(this, "repositoryEnableChanged", Qt::QueuedConnection);
}

void Mod::fetchEta()
{
    int eta = sync_->getFolderEta(key_);
    if (eta == -404)
    {
        return;
    }
    setEta(eta);
}

Repository* Mod::parentItem()
{
    return static_cast<Repository*>(TreeItem::parentItem());
}
