#include <fstream>

#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QStringLiteral>
#include <QThread>

#ifdef Q_OS_WINDOWS
#pragma warning(push, 0)
#endif
#include <libtorrent/bdecode.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_handle.hpp>
#include <libtorrent/session_stats.hpp>
#include <libtorrent/session_status.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/time.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/write_resume_data.hpp>
#ifdef Q_OS_WINDOWS
#pragma warning(pop)
#endif

#include "../../afisynclogger.h"
#include "../../fileutils.h"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "libtorrentapi.h"

using namespace Qt::StringLiterals;

namespace lt = libtorrent;

const int LibTorrentApi::NOT_FOUND = -404;
const QString LibTorrentApi::ERROR_KEY_NOT_FOUND = u"ERROR: not found. key = "_s;
const QString LibTorrentApi::ERROR_SESSION_NULL = u"ERROR: session is null."_s;

LibTorrentApi::LibTorrentApi() :
    QObject(nullptr)
{
    generalInit();
    QMetaObject::invokeMethod(this, &LibTorrentApi::init, Qt::QueuedConnection);
}

LibTorrentApi::LibTorrentApi(const QStringList& deltaUrls):
    QObject(nullptr), // moveToThread cannot be used with parent
    deltaUrls_(deltaUrls)
{
    generalInit();
    if (SettingsModel::deltaPatchingEnabled())
    {
        QMetaObject::invokeMethod(this, &LibTorrentApi::initDelta, Qt::QueuedConnection);
        return;
    }
    QMetaObject::invokeMethod(this, &LibTorrentApi::init, Qt::QueuedConnection);
}

// Initializes LibTorrentApi with delta patching enabled
void LibTorrentApi::initDelta()
{
    generalThreadInit();
    createDeltaManager(deltaUrls_);
    ready_ = true;
    emit initCompleted();
}

void LibTorrentApi::init()
{
    generalThreadInit();
    ready_ = true;
    emit initCompleted();
}

void LibTorrentApi::generalInit()
{
    ready_ = false;
    session_ = nullptr;
    deltaManager_ = nullptr;
    numResumeData_ = 0;
    settingsPath_ = SettingsModel::syncSettingsPath() + "/libtorrent.dat";
    moveToThread(Global::workerThread);
}

qint64 LibTorrentApi::folderTotalWantedMoving(const QString& key)
{
    lt::torrent_handle handle = keyHash_.value(key);
    return 0;
}

void LibTorrentApi::generalThreadInit()
{
    createSession();
    storageMoveManager_ = new StorageMoveManager();
    alertHandler_ = new AlertHandler(this);
    connect(alertHandler_, &AlertHandler::uploadAndDownloadChanged, this, [=] (int64_t ul, int64_t dl) {
        uploadSpeed_ = ul;
        downloadSpeed_ = dl;
    });
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &LibTorrentApi::handleAlerts);
    connect(timer_, &QTimer::timeout, storageMoveManager_, &StorageMoveManager::update);
    timer_->setInterval(1000);
    timer_->start();
    networkAccessManager_ = new QNetworkAccessManager(this);
}

LibTorrentApi::~LibTorrentApi()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    shutdown();
}

void LibTorrentApi::createSession()
{
    LOG;
    lt::settings_pack settings;

    //Disable encryption
    settings.set_int(lt::settings_pack::allowed_enc_level, lt::settings_pack::enc_level::pe_plaintext);
    settings.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::enc_policy::pe_disabled);
    settings.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::enc_policy::pe_disabled);
    //Disable uTP, enable TCP
    settings.set_bool(lt::settings_pack::enable_incoming_tcp, true);
    settings.set_bool(lt::settings_pack::enable_outgoing_tcp, true);
    settings.set_bool(lt::settings_pack::enable_incoming_utp, false);
    settings.set_bool(lt::settings_pack::enable_outgoing_utp, false);
    //Attempt to fix the issue in which only one peer is propelly connected.
    settings.set_int(lt::settings_pack::unchoke_slots_limit, 100);

    //Change user agent
    std::string userAgent = "AFISync";
    if (Global::guiless)
    {
        userAgent = "AFISync_Mirror";
        //Try to maximize upload speed
        settings.set_int(lt::settings_pack::connections_limit, 1000);
        settings.set_int(lt::settings_pack::seed_choking_algorithm, lt::settings_pack::fastest_upload);
    }
    settings.set_str(lt::settings_pack::user_agent, userAgent + "/" + Constants::VERSION_STRING.toStdString());
    //Load port setting
    QString port = SettingsModel::portEnabled() ? SettingsModel::port() : Constants::DEFAULT_PORT;
    settings.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:" + port.toStdString());
    //Load bandwidth limits
    const QString uLimit = SettingsModel::maxUpload();
    const QString dLimit = SettingsModel::maxDownload();
    if (!uLimit.isEmpty())
        settings.set_int(lt::settings_pack::upload_rate_limit, uLimit.toInt() * 1024);
    if (dLimit.isEmpty())
        settings.set_int(lt::settings_pack::download_rate_limit, dLimit.toInt() * 1024);

    //Sanity check
    if (session_)
    {
        LOG_ERROR << "Session is not null.";
        delete session_;
    }
    session_ = new lt::session(settings);
    loadTorrentFiles(SettingsModel::syncSettingsPath());
}

void LibTorrentApi::checkFolder(const QString& key)
{
    LOG << "Checking torrent with key " << key;
    if (deltaManager_ && deltaManager_->contains(key))
    {
        LOG_ERROR << "Torrent is being delta patched. Recheck refused.";
        return;
    }
    lt::torrent_handle h = getHandle(key);
    if (!h.is_valid())
        return;

    h.force_recheck();
}

// TODO: Use mutexes here as keyHash is written in worker thread and read in ui thread.
QStringList LibTorrentApi::folderKeys()
{
    return keyHash_.keys();
}

bool LibTorrentApi::folderNoPeers(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return true;

    lt::torrent_status status = handle.status();
    return status.num_peers == 0 && (status.state == libtorrent::torrent_status::state_t::downloading
            || status.state == libtorrent::torrent_status::state_t::downloading_metadata);
}

bool LibTorrentApi::folderReady(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
        return false;

    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    // is_finished reports incorrent value
    return status.progress == 1;
}

bool LibTorrentApi::folderChecking(const lt::torrent_status& status) const
{
    lt::torrent_status::state_t state = status.state;
    //LOG << "key =" << key << "state =" << state << "active time =" << handle.name().c_str()
    //    << "\n allocating =" << lt::torrent_status::state_t::allocating
    //    << "\n checking files =" << lt::torrent_status::state_t::checking_files
    //    << "\n checking_resume_data =" << lt::torrent_status::state_t::checking_resume_data
    //    << "\n queued_for_checking =" << lt::torrent_status::state_t::queued_for_checking;
    return (state == lt::torrent_status::state_t::checking_files
            || state == lt::torrent_status::state_t::checking_resume_data);
}

bool LibTorrentApi::folderChecking(const QString& key)
{
    lt::torrent_handle handle = getHandleSilent(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    return folderChecking(status);
}

bool LibTorrentApi::folderDownloading(const QString& key)
{
    if (torrentDownloading_.contains(key))
    {
        return true;
    }
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return false;
    }
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    return status.state == lt::torrent_status::state_t::downloading ||
            status.state == lt::torrent_status::downloading_metadata;
}

bool LibTorrentApi::folderMovingFiles(const QString& key)
{
    return storageMoveManager_->contains(key);
}

bool LibTorrentApi::folderQueued(const lt::torrent_status& status) const
{
    //Torrent is always paused when queued
    if (status.is_finished || !(status.flags & lt::torrent_flags::auto_managed))
        return false;

    bool checkingQueued = folderChecking(status) && status.queue_position > 1; //Torrent is paused+checking
    bool downloadQueued = status.state == lt::torrent_status::downloading && status.queue_position > 3;
    return checkingQueued || downloadQueued;
}

//Overly complicated function which returns true if folder is queued for anything.
//libTorrent does not seem to have a proper way to query this...
bool LibTorrentApi::folderQueued(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return deltaManager_->queued(key);
    }
    if (!keyHash_.contains(key)) {
        return false;
    }

    lt::torrent_handle handle = keyHash_.value(key);
    lt::torrent_status status = handle.status();
    return folderQueued(status);
}

void LibTorrentApi::setDeltaUrls(const QStringList& urls)
{
    QMetaObject::invokeMethod(this, "setDeltaUrlsSlot",
                              Qt::QueuedConnection, Q_ARG(QStringList, urls));
}

void LibTorrentApi::setFolderPath(const QString& key, const QString& path)
{
    lt::torrent_handle handle = getHandle(key);
    auto status = handle.status(lt::torrent_handle::query_save_path | lt::torrent_handle::query_name);
    const QString name = QString::fromStdString(status.name);
    const QString savePath = QString::fromStdString(status.save_path);
    // Path can be drive root in which case it ends with "\" for example "C:\"
    const QString fromPath = QDir::fromNativeSeparators(savePath + (savePath.endsWith('\\') ? QString() : u"\\"_s) + name);
    const QString toPath = QDir::fromNativeSeparators(path + (path.endsWith('\\') ? QString() : u"\\"_s) + name);
    storageMoveManager_->insert(key, fromPath, toPath, handle.status().total_wanted_done);
    handle.move_storage(path.toStdString());
}

void LibTorrentApi::setFolderPaused(const QString& key, bool value)
{
    lt::torrent_handle handle = getHandle(key);
    QString name = QString::fromStdString(handle.status().name);
    if (value)
    {
        handle.unset_flags(lt::torrent_flags::auto_managed);
        handle.pause();
        LOG << name << "paused";
    }
    else
    {
        LOG << "Starting torrent " << name;
        handle.resume();
        if (Global::guiless)
        {
            handle.unset_flags(lt::torrent_flags::auto_managed);
            LOG << "Auto management disabled for " << name << " in mirror mode.";
        }
        else
        {
            handle.set_flags(lt::torrent_flags::auto_managed);
        }
    }
}

bool LibTorrentApi::folderCheckingPatches(const QString& key)
{
    return deltaManager_ && deltaManager_->contains(key) && folderChecking(key);
}

QPair<lt::error_code, lt::add_torrent_params> LibTorrentApi::toAddTorrentParams(const QByteArray& torrentData) {
    Q_ASSERT(!torrentData.isEmpty());
    lt::add_torrent_params atp;
    lt::error_code ec;
    auto ptr = new lt::torrent_info(torrentData.constData(), torrentData.size(), ec);
    if (ec.failed())
    {
        delete ptr;
        return {ec, atp};
    }
    atp.ti = std::shared_ptr<lt::torrent_info>(ptr);

    QFileInfo fi(SettingsModel::modDownloadPath());
    atp.save_path = QDir::toNativeSeparators(fi.absoluteFilePath()).toStdString();
    atp.flags |= libtorrent::torrent_flags::paused;
    return {ec, atp};
}

bool LibTorrentApi::folderPatching(const QString& key)
{
    return deltaManager_ && deltaManager_->patching(key);
}

bool LibTorrentApi::folderDownloadingPatches(const QString& key)
{
    return deltaManager_ && deltaManager_->patchDownloading(key);
}

bool LibTorrentApi::folderExtractingPatch(const QString& key)
{
    return deltaManager_ && deltaManager_->patchExtracting(key);
}

void LibTorrentApi::disableQueue(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return;

    handle.queue_position_top();

    handle.unset_flags(lt::torrent_flags::auto_managed);
    handle.resume();
}

qint64 LibTorrentApi::folderTotalWanted(const QString& key)
{
    if (torrentDownloading_.contains(key))
    {
        return -1;
    }
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return deltaManager_->totalWantedDone(key);
    }
    if (folderMovingFiles(key))
    {
        return storageMoveManager_->totalWanted(key);
    }

    lt::torrent_handle handle = getHandle(key);
    lt::torrent_status status = handle.status();
    return status.state == lt::torrent_status::downloading_metadata ? -1 : status.total_wanted;
}

qint64 LibTorrentApi::folderTotalWantedDone(const QString& key)
{
    if (torrentDownloading_.contains(key))
    {
        return -1;
    }
    if (folderMovingFiles(key))
    {
        return storageMoveManager_->totalWantedDone(key);
    }
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return deltaManager_->totalWantedDone(key);
    }

    const lt::torrent_status status = getHandle(key).status();
    if (status.state == lt::torrent_status::downloading_metadata)
    {
        return -1;
    }
    // Circumvent bug within libTorrent.
    if (status.total_wanted_done == 0 && folderChecking(status)) {
        return status.progress * status.total_wanted;
    }
    return status.total_wanted_done;
}

// Cleans unused torrent, link and resume datas
void LibTorrentApi::cleanUnusedFiles(const QSet<QString>& usedKeys)
{
    const auto keys = torrentParams_.keys();
    for (const QString& key : keys)
    {
        if (!usedKeys.contains(key) && !deltaUrls_.contains(key))
        {
            removeFiles(prefixMap_.find(key).value());
            torrentParams_.remove(key); // No longer valid
        }
    }
}

int64_t LibTorrentApi::bytesToCheck(const lt::torrent_status& status) const
{
    if (!session_)
        return NOT_FOUND;

    int64_t tw = status.total_wanted;
    if (tw == 0) //Not loaded yet.
        return NOT_FOUND;

    int64_t rVal = tw - status.total_done;

    //LOG << "rVal =" << rVal;
    return rVal;
}

//In early states torrent_file is null.
//This function waits for 5s at most for it to get
//initialized.
std::shared_ptr<const lt::torrent_info> LibTorrentApi::getTorrentFile(
        const lt::torrent_handle& handle) const
{
    auto torrentFile = handle.torrent_file();
    for (int i = 0; !torrentFile; ++i)
    {
        QThread::sleep(1);
        torrentFile = handle.torrent_file();
        if (i >= 5)
            return nullptr;
    }
    return torrentFile;
}

QSet<QString> LibTorrentApi::folderFilesUpper(const QString& key)
{
    QSet<QString> rVal;
    lt::torrent_handle handle = keyHash_.value(key);
    if (!handle.is_valid())
        return rVal;

    auto torrentFile = getTorrentFile(handle);
    if (!torrentFile)
    {
        LOG_ERROR << "torrent_file is null";
        return rVal;
    }
    lt::file_storage files = torrentFile->files();
    for (int i = 0; i < files.num_files(); ++i)
    {
        lt::torrent_status status = handle.status(lt::torrent_handle::query_save_path);
        QString value = QString::fromStdString(status.save_path + "/" + files.file_path(i));
        value = QFileInfo(value).absoluteFilePath().toUpper();
        rVal.insert(value);
    }
    return rVal;
}

bool LibTorrentApi::folderExists(const QString& key)
{
    for (const auto& pending : pendingFolder_) {
        if (pending.first == key) {
            return true;
        }
    }
    if (torrentDownloading_.contains(key) || (deltaManager_ && deltaManager_->contains(key)))
    {
        return true;
    }
    lt::torrent_handle handle = getHandleSilent(key);
    return handle.is_valid();
}

bool LibTorrentApi::folderPaused(const QString& key)
{
    lt::torrent_handle handle = getHandleSilent(key);
    if (!handle.is_valid())
        return false;

    return handle.flags() & lt::torrent_flags::paused;
}

QString LibTorrentApi::folderError(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return {};

    auto errc = handle.status().errc;
    if (!errc)
    {
        return {};
    }
    QString rVal = QString::fromStdString(handle.status().errc.message());
    return rVal;
}

QString LibTorrentApi::folderPath(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return {};

    lt::torrent_status status = handle.status();
    QString rVal = QDir::fromNativeSeparators(QString::fromStdString(status.save_path)) + "/";
    if (deltaManager_ && deltaManager_->contains(key))
    {
        rVal += deltaManager_->name(key);
    }
    else
    {
        rVal += QString::fromStdString(status.name);
    }
    return rVal;
}

//Shutdowns libtorrent session and saves settings.
void LibTorrentApi::shutdown()
{
    bool deleteSession = true;

    timer_->stop();
    LOG << "Alert timer stopped";
    generateResumeData();
    const auto keys = keyHash_.keys();
    for (const QString& key : keys)
    {
        lt::torrent_handle handle = keyHash_.value(key);
        if (LibTorrentApi::folderExists(key))
        {
            saveTorrentFile(handle);
        }
        else
        {
            //Still downloading meta-data. Propably incorrect URL.
            //Work-a-round: Hangs in delete so don't delete...
            //FIXME: Find better solution.
            LOG_ERROR << key << " does not exist. Not deleting session as it would hang in deconstruction (BUG).";
            deleteSession = false;
        }
        LOG << "Removing " << key << " from sync";
        session_->remove_torrent(handle);
    }
    if (deltaManager_)
    {
        const QList<lt::torrent_handle> handles = deltaManager_->handles();
        for (const auto& handle : handles) {
            auto file = handle.torrent_file();
            auto status = handle.status();
            saveTorrentFile(handle);
            LOG << "Delta Download torrent saved.";
        }
    }
    delete deltaManager_;
    deltaManager_ = nullptr;
    LOG << "Settings saved";
    deleteSession = deleteSession && storageMoveManager_->inactive();
    delete storageMoveManager_;
    if (session_ && deleteSession)
    {
        //FIXME: Hangs here if downloading metadata.
        delete session_;
        session_ = nullptr;
        LOG << "Session deleted";
    }
    LOG << "Shutdown completed.";
    emit shutdownCompleted();
}

int64_t LibTorrentApi::upload()
{
    if (!session_)
        return 0;

    return uploadSpeed_;
}

int64_t LibTorrentApi::download() const
{
    if (!session_)
        return 0;

    return downloadSpeed_;
}

void LibTorrentApi::setMaxUpload(const int limit)
{
    QMetaObject::invokeMethod(this, "setMaxUploadSlot", Qt::QueuedConnection, Q_ARG(int, limit));
}


void LibTorrentApi::setMaxUploadSlot(const int limit)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::upload_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

void LibTorrentApi::setMaxDownload(const int limit)
{
    QMetaObject::invokeMethod(this, "setMaxDownloadSlot", Qt::QueuedConnection, Q_ARG(int, limit));
}

void LibTorrentApi::setMaxDownloadSlot(const int limit)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::download_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

bool LibTorrentApi::ready()
{
    return ready_;
}

void LibTorrentApi::setPort(int port)
{
    QMetaObject::invokeMethod(this, "setPortSlot", Qt::QueuedConnection, Q_ARG(int, port));
}

void LibTorrentApi::setPortSlot(int port)
{
    if (!session_)
    {
        LOG_ERROR << "session is null";
        return;
    }

    LOG << "port = " << port;
    if (port < 0)
    {
        LOG << "Incorrect port " << port << " given";
        return;
    }
    lt::settings_pack pack;
    pack.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:" + QString::number(port).toStdString());
    session_->apply_settings(pack);
}

void LibTorrentApi::handleAlerts()
{
    if (!session_)
    {
        LOG << ERROR_SESSION_NULL;
        return;
    }

    session_->post_session_stats();
    auto alerts = new std::vector<lt::alert*>();
    session_->pop_alerts(alerts);
    alertHandler_->handleAlerts(alerts);
    delete alerts;
}

QStringList LibTorrentApi::deltaUrls()
{
    return deltaUrls_;
}

void LibTorrentApi::removeFiles(const QString& hashString)
{
    QString filePrefix = SettingsModel::syncSettingsPath() + "/" + hashString;
    //Delete saved data
    QString urlPath = filePrefix + ".link";
    FileUtils::safeRemove(urlPath);
    QString torrentPath = filePrefix + ".torrent";
    FileUtils::safeRemove(torrentPath);
    QString fastresumePath = filePrefix + ".fastresume";
    FileUtils::safeRemove(fastresumePath);
}

void LibTorrentApi::removeFolder(const QString& key)
{
    QMetaObject::invokeMethod(this, "removeFolderSlot", Qt::QueuedConnection, Q_ARG(QString, key));
}

bool LibTorrentApi::removeFolderSlot(const QString& key)
{
    LOG << "key = " << key;
    if (!session_)
        return false;

    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        lt::torrent_status sta = handle.status();
        LOG_ERROR << "Torrent is in invalid state. error = "
                  << QString::fromStdString(sta.errc.message())
                  << " state = " << sta.state;
        return false;
    }
    LOG << "Removing " << key << " from sync";
    session_->remove_torrent(handle);
    removeFiles(getHashString(handle));
    keyHash_.remove(key);
    std::vector<lt::alert*>* alerts = new std::vector<lt::alert*>();
    //Wait for removed alert (5s)
    //50 alert chunks may be received before torrent removed alert.
    for (int i = 0; i < 50; ++i)
    {
        session_->wait_for_alert(lt::milliseconds(100));
        session_->pop_alerts(alerts);
        for (const lt::alert* alert : *alerts)
        {
            if (alert->type() == lt::torrent_removed_alert::alert_type)
            {
                LOG << "Removed alert received. Returning...";
                delete alerts;
                return true;
            }
        }
    }
    delete alerts;
    return false;
}

void LibTorrentApi::mirrorDeltaPatches()
{
    QMetaObject::invokeMethod(deltaManager_, &DeltaManager::mirrorDeltaPatches, Qt::QueuedConnection);
}

void LibTorrentApi::setDeltaUpdatesFolder(const QString& key)
{
    // FIXME: QMetaObject::invokeMethod: No such method
    // LibTorrentApi::setDeltaUpdatesFolderSlot(QString)
    Q_ASSERT(false);
    QMetaObject::invokeMethod(this, "setDeltaUpdatesFolderSlot", Qt::QueuedConnection, Q_ARG(QString, key));
}

void LibTorrentApi::setDeltaUrlsSlot(const QStringList& deltaUrls)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (deltaUrls == deltaUrls_)
    {
        return;
    }
    deltaUrls_ = deltaUrls;
    enableDeltaUpdatesSlot();
    deltaManager_->setDeltaUrls(deltaUrls);
}

bool LibTorrentApi::disableDeltaUpdates()
{
    if (!deltaManager_)
        return false;

    CiHash<QString> hash = deltaManager_->keyHash();
    disableDeltaUpdatesNoTorrents();
    //Enable regular torrent downloads
    const auto keys = hash.keys();
    for (const QString& key : keys)
        addFolder(key, hash.value(key));

    return true;
}

void LibTorrentApi::disableDeltaUpdatesNoTorrents()
{
    LOG;
    delete deltaManager_;
    deltaManager_ = nullptr;
}

void LibTorrentApi::enableDeltaUpdates()
{
    QMetaObject::invokeMethod(this, &LibTorrentApi::enableDeltaUpdatesSlot, Qt::QueuedConnection);
}

void LibTorrentApi::enableDeltaUpdatesSlot()
{
    LOG;
    if (deltaManager_ || creatingDeltaManager_)
    {
        LOG << "Delta updates already active, doing nothing.";
        return;
    }
    deltaManager_ = new DeltaManager(deltaUrls_, session_, this);
    connect(deltaManager_, &DeltaManager::patched, this, &LibTorrentApi::handlePatched);
}

void LibTorrentApi::addFolderGeneric(const QString& key)
{
    addFolderGenericAsync(key);
}

lt::torrent_handle LibTorrentApi::addFolderFromParams(const QString& key)
{
    auto it = torrentParams_.find(key);
    if (it != torrentParams_.end())
    {
        LOG << "Adding " << key << " to sync";
        lt::error_code ec;
        lt::torrent_handle handle = session_->add_torrent(it.value(), ec);
        if (ec.failed())
        {
            LOG_ERROR << "Unable to read torrent params for " << key << " " << ec.message();
            return lt::torrent_handle{};
        }
        auto status = handle.status();
        if (deltaManager_ && !deltaManager_->contains(key))
            keyHash_.insert(key, handle);
        return handle;
    }

    return lt::torrent_handle();
}

//Does not wait for torrent-file download to finish.
bool LibTorrentApi::addFolderGenericAsync(const QString& key)
{
    lt::torrent_handle existingHandle = addFolderFromParams(key);
    if (existingHandle.is_valid())
    {
        onFolderAdded(key, existingHandle);
        return true;
    }

    // TODO: Remove? Might be too defensive
    if (!session_)
    {
        LOG << ERROR_SESSION_NULL;
        onFolderAdded(key, lt::torrent_handle());
        return false;
    }

    QFileInfo fi(SettingsModel::modDownloadPath());
    QDir().mkpath(fi.absoluteFilePath());
    if (!fi.isReadable())
    {
        LOG_ERROR << "Parent dir " << fi.absoluteFilePath() << " is not readable.";
        onFolderAdded(key, lt::torrent_handle());
        return false;
    }
    if (folderExists(key))
    {
        LOG_ERROR << "Torrent already added.";
        onFolderAdded(key, lt::torrent_handle());
        return false;
    }

    auto reply = networkAccessManager_->get(QNetworkRequest(QUrl(key)));
    torrentDownloading_.insert(key);
    LOG << "Downloading torrent " << key << " ...";
    connect(reply, &QNetworkReply::finished, this, [=] () {
        torrentDownloading_.remove(key);
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR << "Torrent download failed: " << reply->errorString();
            return;
        }
        LOG << key << " downloaded";

        QByteArray torrentData = reply->readAll();
        reply->deleteLater();

        QPair<lt::error_code, lt::add_torrent_params> pair = toAddTorrentParams(torrentData);
        if (pair.first.failed())
        {
            LOG_WARNING << "Could not convert downloaded data to torrent params: "
                        << pair.first.message();
            return;
        }
        auto atp = pair.second;
        lt::error_code ec;
        lt::torrent_handle handle = session_->add_torrent(atp, ec);
        if (ec)
        {
            LOG_ERROR << "Adding torrent failed: " << ec.message().c_str();
            return;
        }
        onFolderAdded(key, handle);
    });
    return false;
}

//Does not print error message.
lt::torrent_handle LibTorrentApi::getHandleSilent(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it != keyHash_.end())
        return it.value();

    if (deltaManager_ && deltaManager_->contains(key))
        return deltaManager_->getHandle(key);

    return lt::torrent_handle();
}

lt::torrent_handle LibTorrentApi::getHandle(const QString& key)
{
    lt::torrent_handle rVal = getHandleSilent(key);
    if (!rVal.is_valid())
    {
        LOG << ERROR_KEY_NOT_FOUND << key;
    }

    return rVal;
}

bool LibTorrentApi::addFolder(const QString& key, const QString& name)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (!deltaManager_ && !deltaUrls_.isEmpty() && SettingsModel::deltaPatchingEnabled())
    {
        // Wait for the delta torrent to be processed before adding
        // mod torrents.
        pendingFolder_.enqueue({key, name});
        LOG << "Folder add postponed " << key << " " << name;
        return false;
    }
    else
    {
        return addFolder(key, name, true);
    }
}

bool LibTorrentApi::addFolder(const QString& key, const QString& name, bool patchingEnabled)
{
    if (SettingsModel::deltaPatchingEnabled() &&
            patchingEnabled && deltaManager_ && deltaManager_->patchAvailable(name))
    {
        deltaManager_->patch(name, key);
        return false;
    }
    return addFolderGenericAsync(key);
    // continues at onFolderAdded
}

void LibTorrentApi::onFolderAdded(const QString& key, lt::torrent_handle handle) {
    keyHash_.insert(key, handle);
    handle.resume();
    emit folderAdded(key);
}

void LibTorrentApi::handlePatched(const QString& key, const QString& modName, bool success)
{
    Q_UNUSED(success)

    LOG << "Patching done. Staring the mod torrent ... " << key << " " << modName << " " << success;
    //Re-add folder and prevent infinite loop by patch refusal.
    addFolderGenericAsync(key);
}

QString LibTorrentApi::getHashString(const lt::torrent_handle& handle) const
{
    lt::torrent_status status = handle.status();
    auto sha1Hash = status.info_hashes.v1;
    auto hex = QByteArray::fromRawData(sha1Hash.data(), sha1Hash.size()).toHex();
    auto hash = QString::fromLatin1(hex);
    LOG << "hash = " << hash << " name = " << QString::fromStdString(status.name);
    return hash;
}

bool LibTorrentApi::saveTorrentFile(const lt::torrent_handle& handle) const
{
    QString filePrefix = SettingsModel::syncSettingsPath() + "/" + getHashString(handle);
    //Torrent file
    auto ti = getTorrentFile(handle);
    LOG << "name = " << QString::fromStdString(handle.status(lt::torrent_handle::query_name).name);
    if (!ti)
    {
        LOG_ERROR << "Unable to save torrent file. It is null.";
        return false;
    }
    if (!ti->is_valid())
    {
        LOG_ERROR << "Unable to save torrent file. It is invalid.";
        return false;
    }
    lt::create_torrent new_torrent(*ti);
    QByteArray torrentBytes;
    bencode(std::back_inserter(torrentBytes), new_torrent.generate());
    QString torrentFilePath = filePrefix + ".torrent";
    QString urlFilePath = filePrefix + ".link";
    QByteArray url = keyHash_.key(handle).toUtf8();
    if (url.isEmpty()) {
        url = deltaManager_->getUrl(handle).toUtf8();
    }
    return FileUtils::writeFile(torrentBytes, torrentFilePath) && FileUtils::writeFile(url, urlFilePath);
}

void LibTorrentApi::generateResumeData() const
{
    LOG;
    if (!session_)
        return;

    static int outstanding_resume_data = 0;
    std::vector<lt::torrent_handle> handles = session_->get_torrents();
    session_->pause();
    for (std::vector<lt::torrent_handle>::iterator i = handles.begin();
            i != handles.end(); ++i)
    {
        lt::torrent_handle& h = *i;
        if (!h.is_valid()) continue;
        lt::torrent_status s = h.status();
        if (!s.has_metadata || !s.need_save_resume) continue;

        h.save_resume_data();
        ++outstanding_resume_data;
    }

    while (outstanding_resume_data > 0)
    {
        lt::alert* test = session_->wait_for_alert(lt::seconds(10));

        // if we don't get an alert within 10 seconds, abort
        if (test == 0) break;

        std::vector<lt::alert*>* alerts = new std::vector<lt::alert*>();
        session_->pop_alerts(alerts);

        for (lt::alert* a : *alerts)
        {
            if (lt::alert_cast<lt::save_resume_data_failed_alert>(a))
            {
                LOG << "Failure in saving resume data: " << a->message().c_str();
                --outstanding_resume_data;
                continue;
            }

            lt::save_resume_data_alert const* rd = lt::alert_cast<lt::save_resume_data_alert>(a);
            if (rd == 0)
                continue;

            lt::torrent_handle h = rd->handle;
            auto resumeData = lt::write_resume_data(rd->params);
            std::ofstream out((SettingsModel::syncSettingsPath().toStdString()
                               + "/" + getHashString(h).toStdString() + ".fastresume").c_str()
                    , std::ios_base::binary);
            out.unsetf(std::ios_base::skipws);
            bencode(std::ostream_iterator<char>(out), resumeData);
            --outstanding_resume_data;
            LOG << "Resume data generated for " << h.status().name.c_str();
        }
        LOG << "Deleting alerts";
        delete alerts;
    }
}

void LibTorrentApi::createDeltaManager(const QStringList& deltaUrls)
{
    if (deltaManager_)
    {
        LOG << "Replacing old deltaManager " << deltaUrls_
            << " with " << deltaUrls;

        for (const auto& url  : std::as_const(deltaUrls_)) {
            removeFolder(url);
        }
        delete deltaManager_;
        deltaManager_ = nullptr;
    }

    deltaManager_ = new DeltaManager(deltaUrls, session_, this);
    deltaUrls_ = deltaUrls;
    connect(deltaManager_, &DeltaManager::patched, this, &LibTorrentApi::handlePatched);
}

// TODO: Instead of loading, simply cache.
void LibTorrentApi::loadTorrentFiles(const QDir& dir)
{
    if (!dir.isReadable())
    {
        LOG_WARNING << "Directory " << dir.absolutePath() << " is not readable.";
        return;
    }

    QDirIterator it(dir.absolutePath(), QDir::Files);
    //Combine resume and torrent datas
    while (it.hasNext())
    {
        QString filePath = it.filePath();
        if (!filePath.endsWith(".torrent"_L1)) {
            it.next();
            continue;
        }
        LOG << "Processing: " << filePath;

        QString pathPrefix = filePath.remove(u".torrent"_s);
        // Load resume data
        QByteArray bytes = FileUtils::readFile(pathPrefix + ".fastresume");
        if (bytes.isEmpty()) {
            it.next();
            continue;
        }
        auto bData = lt::bdecode(bytes);
        lt::add_torrent_params params = lt::read_resume_data(bData);

        params.flags |= lt::torrent_flags::auto_managed;
        params.ti = loadFromFile(pathPrefix + ".torrent");
        params.save_path = SettingsModel::modDownloadPath().toStdString();
        //FIXME: Sometimes this becomes off as empty.
        QString url = QString::fromLocal8Bit(FileUtils::readFile(pathPrefix + ".link")).toLower();
        prefixMap_.insert(url, it.fileName().remove(u".torrent"_s));
        if (params.ti == 0 || url.isEmpty() || !params.ti->is_valid())
        {
            LOG_ERROR << "Loading torrent " << filePath << " url = " << url;
            it.next();
            continue;
        }
        QString name = QString::fromStdString(params.ti->name());
        LOG << "Appending " << name;
        torrentParams_.insert(url, params); // Used in addFolder()
        it.next();
    }
}

std::shared_ptr<lt::torrent_info> LibTorrentApi::loadFromFile(const QString& path) const
{
    lt::error_code ec;
    std::string np = QDir::toNativeSeparators(path).toStdString();
    auto info = std::shared_ptr<lt::torrent_info>(new lt::torrent_info(np, ec));
    if (ec)
    {
        QString error = QString::fromUtf8(ec.message().c_str());
        LOG << "Cannot load .torrent file: " << error << " path = " << path;
        return 0;
    }

    return info;
}
