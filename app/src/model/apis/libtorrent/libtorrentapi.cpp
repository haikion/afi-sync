#include <fstream>
#include <map>

#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#pragma warning(push, 0)
#include <libtorrent/alert_manager.hpp>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/lazy_entry.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/session_handle.hpp>
#include <libtorrent/session_status.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/time.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/session_stats.hpp>
#pragma warning(pop)

#include "../../fileutils.h"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "libtorrentapi.h"

namespace lt = libtorrent;

const int LibTorrentApi::NOT_FOUND = -404;
const QString LibTorrentApi::ERROR_KEY_NOT_FOUND = "ERROR: not found. key = ";
const QString LibTorrentApi::ERROR_SESSION_NULL = "ERROR: session is null.";

LibTorrentApi::LibTorrentApi() :
    QObject(nullptr)
{
    generalInit();
    QMetaObject::invokeMethod(this, &LibTorrentApi::init, Qt::QueuedConnection);
}

LibTorrentApi::LibTorrentApi(const QString& deltaUpdatesKey):
    QObject(nullptr), // moveToThread cannot be used with parent
    deltaUpdatesKey_(deltaUpdatesKey)
{
    generalInit();
    if (SettingsModel::deltaPatchingEnabled())
    {
        QMetaObject::invokeMethod(this, &LibTorrentApi::initDelta, Qt::QueuedConnection);
        return;
    }
    QMetaObject::invokeMethod(this, &LibTorrentApi::init, Qt::QueuedConnection);
}

void LibTorrentApi::initDelta()
{
    generalThreadInit();
    enableDeltaUpdatesSlot();
    emit initCompleted();
}

void LibTorrentApi::init()
{
    generalThreadInit();
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
    connect(alertHandler_, &AlertHandler::uploadAndDownloadChanged, this, [=] (quint64 ul, quint64 dl) {
        uploadSpeed_ = ul;
        downloadSpeed_ = dl;
    });
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &LibTorrentApi::handleAlerts);
    connect(timer_, &QTimer::timeout, storageMoveManager_, &StorageMoveManager::update);
    timer_->setInterval(1000);
    timer_->start();
    ready_ = true;
}

LibTorrentApi::~LibTorrentApi()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    shutdown();
}

bool LibTorrentApi::createSession()
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
    if (uLimit != "")
        settings.set_int(lt::settings_pack::upload_rate_limit, uLimit.toInt() * 1024);
    if (dLimit != "")
        settings.set_int(lt::settings_pack::download_rate_limit, dLimit.toInt() * 1024);

    //Sanity check
    if (session_)
    {
        LOG_ERROR << "Session is not null.";
        delete session_;
    }
    session_ = new lt::session(settings);
    bool rVal = loadLtSettings();
    loadTorrentFiles(SettingsModel::syncSettingsPath());
    return rVal;
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

void LibTorrentApi::saveSettings()
{
    if (!session_)
        return;

    lt::entry e(lt::entry::data_type::dictionary_t);
    session_->save_state(e);
    auto map = e.dict();
    QByteArray bytes;
    lt::bencode(std::back_inserter(bytes), e);
    FileUtils::writeFile(bytes, settingsPath_);
    LOG << "Data saved. " << bytes.size() << " bytes written.";
}

//Loads libTorrent specific settings
bool LibTorrentApi::loadLtSettings()
{
    LOG;
    if (!session_)
        return false;

    QByteArray bytes = FileUtils::readFile(settingsPath_);
    if (bytes.size() == 0)
        return false;
    lt::bdecode_node fast;
    lt::error_code ec;
    lt::bdecode(bytes.constData(), bytes.constData() + bytes.size(), fast, ec);
    LOG << bytes.size() << " bytes read.";
    if (ec || (fast.type() != lt::bdecode_node::dict_t))
    {
        LOG << "Failure";
        return false;
    }
    session_->load_state(fast);
    LOG << "Data loaded";
    return true;
}

// TODO: Use mutexes here as keyHash is written in worker thread and read in ui thread.
QList<QString> LibTorrentApi::folderKeys()
{
    QList<QString> rVal = keyHash_.keys();
    if (deltaManager_)
        rVal += deltaManager_->folderKeys();

    return rVal;
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
         || state == lt::torrent_status::state_t::checking_resume_data
         || state == lt::torrent_status::state_t::allocating);
}

bool LibTorrentApi::folderChecking(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    return folderChecking(status);
}

bool LibTorrentApi::folderDownloading(const QString& key)
{
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
    lt::torrent_status::state_t state = status.state;

    //Torrent is always paused when queued
    if (!status.auto_managed || status.is_finished || !status.paused)
        return false;

    //Default active downloads is 3.
    //LOG << "key =" << key << "state =" << state << "name =" << handle.name().c_str()
    //    << "\n allocating =" << lt::torrent_status::state_t::allocating
    //    << "\n checking files =" << lt::torrent_status::state_t::checking_files
    //    << "\n checking_resume_data =" << lt::torrent_status::state_t::checking_resume_data
    //    << "\n queued_for_checking =" << lt::torrent_status::state_t::queued_for_checking
    //    << "\n pos =" << status.queue_position;
    //Auto manager pauses torrent when queued. status.queue_position cannot be used
    //as activation threshold may change for arbitary reasons.
    bool checkingQueued = folderChecking(status) //Torrent is paused+checking
            //This state is usually not set when queued for checking...
            || state == lt::torrent_status::state_t::queued_for_checking;
    bool downloadQueued = state == lt::torrent_status::downloading;
    return checkingQueued || downloadQueued;
}

//Overly complicated function which returns true if folder is queued for anything.
//libTorrent does not seem to have a proper way to query this...
bool LibTorrentApi::folderQueued(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return deltaManager_->queued(key);;
    }

    lt::torrent_handle handle = keyHash_.value(key);
    lt::torrent_status status = handle.status();
    return folderQueued(status);
}

void LibTorrentApi::setFolderPath(const QString& key, const QString& path)
{
    lt::torrent_handle handle = getHandle(key);
    auto status = handle.status(lt::torrent_handle::query_save_path | lt::torrent_handle::query_name);
    const QString name = QString::fromStdString(status.name);
    const QString savePath = QString::fromStdString(status.save_path);
    // Path can be drive root in which case it ends with "\" for example "C:\"
    const QString fromPath = QDir::fromNativeSeparators(savePath + (savePath.endsWith("\\") ? "" : "\\") + name);
    const QString toPath = QDir::fromNativeSeparators(path + (path.endsWith("\\") ? "" : "\\") + name);
    storageMoveManager_->insert(key, fromPath, toPath, handle.status().total_wanted_done);
    handle.move_storage(path.toStdString());
}

void LibTorrentApi::setFolderPaused(const QString& key, bool value)
{
    lt::torrent_handle handle = getHandle(key);
    QString name = QString::fromStdString(handle.status().name);
    if (value)
    {
        LOG << "Pausing torrent " << name;
        handle.auto_managed(false);
        handle.pause();
        LOG << handle.status().paused;
    }
    else
    {
        LOG << "Starting torrent " << name;
        handle.resume();
        if (Global::guiless)
        {
            handle.auto_managed(false);
            LOG << "Auto management disabled for " << name << " in mirror mode.";
        }
        else
        {
            handle.auto_managed(true);
        }
    }
}

bool LibTorrentApi::folderCheckingPatches(const QString& key)
{
    return deltaManager_ && deltaManager_->contains(key) && folderChecking(key);
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
    handle.auto_managed(false);
    handle.resume();
}

qint64 LibTorrentApi::folderTotalWanted(const QString& key)
{
    const lt::torrent_handle handle = getHandle(key);
    const lt::torrent_status status = handle.status();

    if (folderMovingFiles(key))
    {
        return storageMoveManager_->totalWanted(key);
    }

    if (deltaManager_ && deltaManager_->contains(key))
    {
        return folderChecking(status) ? handle.torrent_file()->total_size() : deltaManager_->totalWanted(key);
    }

    return status.state == lt::torrent_status::downloading_metadata ? -1 : status.total_wanted;
}

qint64 LibTorrentApi::folderTotalWantedDone(const QString& key)
{
    if (folderMovingFiles(key))
    {
        return storageMoveManager_->totalWantedDone(key);
    }

    const lt::torrent_status status = getHandle(key).status();
    if (deltaManager_ && deltaManager_->contains(key))
    {
        // total_wanted_done is 0 when checking
        return folderChecking(status) ? status.total_done : deltaManager_->totalWantedDone(key);
    }

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
void LibTorrentApi::cleanUnusedFiles(const QSet<QString> usedKeys)
{
    for (const QString& key : torrentParams_.keys())
    {
        if (!usedKeys.contains(key) && key != deltaUpdatesKey_)
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
boost::shared_ptr<const lt::torrent_info> LibTorrentApi::getTorrentFile(
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
    LOG << "key = " << key;
    QSet<QString> rVal;
    lt::torrent_handle handle = getHandle(key);
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
    lt::torrent_handle handle = getHandleSilent(key);
    if (!handle.is_valid())
        return false;

    return true;
}

bool LibTorrentApi::folderPaused(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    return status.paused;
}

QString LibTorrentApi::folderError(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return "";

    QString rVal = QString::fromStdString(handle.status().error);
    return rVal;
}

QString LibTorrentApi::folderPath(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return "";

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
    saveSettings();
    generateResumeData();
    for (const QString& key : keyHash_.keys())
    {
        lt::torrent_handle handle = keyHash_.value(key);
        if (folderExists(key))
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
        auto file = deltaManager_->handle().torrent_file();
        auto status = deltaManager_->handle().status();
        saveTorrentFile(deltaManager_->handle());
        delete deltaManager_;
        deltaManager_ = nullptr;
        LOG << "Delta Download torrent saved.";
    }
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

qint64 LibTorrentApi::upload()
{
    if (!session_)
        return 0;

    return session_->status().payload_upload_rate;
    // TODO: Use this instead once libtorrent has been updated
    //return uploadSpeed_;
}

qint64 LibTorrentApi::download() const
{
    if (!session_)
        return 0;

    return session_->status().payload_download_rate;
    // TODO: Use this instead once libtorrent has been updated
    //return downloadSpeed_;
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
    QMetaObject::invokeMethod(this, SLOT(setPortSlot), Qt::QueuedConnection, Q_ARG(int, port));
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

QString LibTorrentApi::deltaUpdatesKey()
{
    return deltaUpdatesKey_;
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
        return false;

    if (!handle.is_valid())
    {
        lt::torrent_status sta = handle.status();
        LOG_ERROR << "Torrent is in invalid state. error = "
            << QString::fromStdString(sta.error)
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

void LibTorrentApi::setDeltaUpdatesFolder(const QString& key)
{
    QMetaObject::invokeMethod(this, "setDeltaUpdatesFolderSlot", Qt::QueuedConnection, Q_ARG(QString, key));
}

void LibTorrentApi::setDeltaUpdatesFolderSlot(const QString& key)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (key == deltaUpdatesKey_)
    {
        return;
    }
    disableDeltaUpdatesNoTorrents();
    deltaUpdatesKey_ = key;
    enableDeltaUpdatesSlot();
}

bool LibTorrentApi::disableDeltaUpdates()
{
    if (!deltaManager_)
        return false;

    CiHash<QString> hash = deltaManager_->keyHash();
    disableDeltaUpdatesNoTorrents();
    //Enable regular torrent downloads
    for (const QString& key : hash.keys())
        addFolder(key, hash.value(key));

    return true;
}

bool LibTorrentApi::disableDeltaUpdatesNoTorrents()
{
    LOG;

    if (!deltaManager_)
        return false;

    delete deltaManager_;
    deltaManager_ = nullptr;
    return true;
}

void LibTorrentApi::enableDeltaUpdates()
{
    QMetaObject::invokeMethod(this, &LibTorrentApi::enableDeltaUpdatesSlot, Qt::QueuedConnection);
}

void LibTorrentApi::enableDeltaUpdatesSlot()
{
    LOG;

    if (deltaUpdatesKey_.isEmpty())
    {
        LOG_ERROR << "Unable to enable delta updates: deltaUpdatesKey_ is empty.";
        return;
    }
    if (deltaManager_)
    {
        LOG << "Delta updates already active, doing nothing.";
        return;
    }
    lt::torrent_handle handle = addFolderFromParams(deltaUpdatesKey_);
    if (!handle.is_valid())
    {
        //Create new deltaManager_;
        handle = addFolderGeneric(deltaUpdatesKey_);
        if (!handle.is_valid())
        {
            LOG_WARNING << "Torrent is invalid. Delta patching disabled.";
            return;
        }
    }

    createDeltaManager(handle, deltaUpdatesKey_);
}

lt::torrent_handle LibTorrentApi::addFolderGeneric(const QString& key)
{
    lt::torrent_handle handle = addFolderGenericAsync(key);
    lt::torrent_status status = handle.status();
    for (int a = 0; a < 100 && status.state == lt::torrent_status::downloading_metadata; ++a)
    {
        QThread::msleep(100);
        status = handle.status();
    }
    if (status.state == lt::torrent_status::downloading_metadata)
    {
        LOG_ERROR << "Failure downloading torrent from " << key << ":" << status.error.c_str();
        return lt::torrent_handle();
    }

    return handle;
}

lt::torrent_handle LibTorrentApi::addFolderFromParams(const QString& key)
{
    auto it = torrentParams_.find(key);
    if (it != torrentParams_.end())
    {
        LOG << "Adding " << key << " to sync";
        lt::torrent_handle handle = session_->add_torrent(it.value());
        auto status = handle.status();
        if (key != deltaUpdatesKey_)
            keyHash_.insert(key, handle);
        return handle;
    }

    return lt::torrent_handle();
}

//Does not wait for torrent-file download to finish.
lt::torrent_handle LibTorrentApi::addFolderGenericAsync(const QString& key)
{
    lt::torrent_handle handle = addFolderFromParams(key);
    if (handle.is_valid())
    {
        return handle;
    }

    // TODO: Remove? Might be too defensive
    if (!session_)
    {
        LOG << ERROR_SESSION_NULL;
        return lt::torrent_handle();
    }

    QFileInfo fi(SettingsModel::modDownloadPath());
    QDir().mkpath(fi.absoluteFilePath());
    if (!fi.isReadable())
    {
        LOG_ERROR << "Parent dir " << fi.absoluteFilePath() << " is not readable.";
        return lt::torrent_handle();
    }
    if (folderExists(key))
    {
        LOG_ERROR << "Torrent already added.";
        return lt::torrent_handle();
    }
    lt::add_torrent_params atp;
    //Check if key is path or url
    if (QFile::exists(key))
    {
        atp.ti = loadFromFile(key);
    }
    else
    {
        atp.url = key.toStdString();
    }
    atp.save_path = QDir::toNativeSeparators(fi.absoluteFilePath()).toStdString();
    atp.flags |= libtorrent::add_torrent_params::flag_paused;
    LOG << "url = " << QString::fromStdString(atp.url)
        << " save_path = " << QString::fromStdString(atp.save_path);
    lt::error_code ec;
    LOG << "Adding " << key << " to sync";
    handle = session_->add_torrent(atp, ec);
    if (ec)
    {
        LOG_ERROR << "Adding torrent failed: " << ec.message().c_str();
        return lt::torrent_handle();
    }

    return handle;
}

//Does not print error message.
lt::torrent_handle LibTorrentApi::getHandleSilent(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it != keyHash_.end())
        return it.value();

    if (deltaManager_ && (key == deltaUpdatesKey_ || deltaManager_->contains(key)))
        return deltaManager_->handle();

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
    return addFolder(key, name, true);
}

bool LibTorrentApi::addFolder(const QString& key, const QString& name, bool patchingEnabled)
{
    if (SettingsModel::deltaPatchingEnabled() &&
            patchingEnabled && deltaManager_ && deltaManager_->patchAvailable(name))
    {
        deltaManager_->patch(name, key);
        return true;
    }
    auto handle = addFolderGenericAsync(key);
    keyHash_.insert(key, handle);
    handle.resume();

    return true;
}

void LibTorrentApi::handlePatched(const QString& key, const QString& modName, bool success)
{
    Q_UNUSED(success)

    LOG << "Patching done. Readding mod. " << key << " " << modName << " " << success;
    //Re-add folder and prevent infinite loop by patch refusal.
    addFolder(key, modName, false);
}

QString LibTorrentApi::getHashString(const lt::torrent_handle& handle) const
{
    lt::torrent_status status = handle.status();
    char out[(libtorrent::sha1_hash::size * 2) + 1];
    lt::to_hex((char const*)&status.info_hash[0], libtorrent::sha1_hash::size, out);
    QString hash = QString(out);
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
    QByteArray url = keyHash_.key(handle).toLocal8Bit();
    if (deltaManager_  && deltaManager_->handle() == handle)
        url = deltaUpdatesKey_.toLocal8Bit();

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

            std::ofstream out((SettingsModel::syncSettingsPath().toStdString()
                               + "/" + getHashString(h).toStdString() + ".fastresume").c_str()
                    , std::ios_base::binary);
            out.unsetf(std::ios_base::skipws);
            bencode(std::ostream_iterator<char>(out), *rd->resume_data);
            --outstanding_resume_data;
            LOG << "Resume data generated for " << h.status().name.c_str();
        }
        LOG << "Deleting alerts";
        delete alerts;
    }
}

void LibTorrentApi::createDeltaManager(lt::torrent_handle handle, const QString& key)
{
    if (deltaManager_)
    {
        LOG << "Replacing old deltaManager " << deltaUpdatesKey_
            << " with " << key;

        removeFolder(deltaUpdatesKey_);
        delete deltaManager_;
    }

    deltaManager_ = new DeltaManager(handle, this);
    deltaUpdatesKey_ = key;

    CiHash<QString> keyHash = deltaManager_->keyHash();
    for (QString key : keyHash)
    {
        QString name = keyHash.value(key);
        LOG << "Retrying patching for " << name << key;
        //Re-apply pending patches.
        deltaManager_->patch(name, key);
    }
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
        if (!filePath.endsWith(".torrent"))
        {
            it.next();
            continue;
        }
        LOG << "Processing: " << filePath;

        QString pathPrefix = filePath.remove(".torrent");
        lt::add_torrent_params params;
        params.ti = loadFromFile(pathPrefix + ".torrent");
        params.save_path = SettingsModel::modDownloadPath().toStdString();
        params.resume_data = loadResumeData(pathPrefix + ".fastresume");
        //FIXME: Sometimes this becomes off as empty.
        QString url = QString::fromLocal8Bit(FileUtils::readFile(pathPrefix + ".link")).toLower();
        prefixMap_.insert(url, it.fileName().remove(".torrent"));
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

std::vector<char> LibTorrentApi::loadResumeData(const QString& path) const
{
    QByteArray bytes = FileUtils::readFile(path);
    std::vector<char> buf(bytes.constData(), bytes.constData() + bytes.size());
    return buf;
}

boost::shared_ptr<lt::torrent_info> LibTorrentApi::loadFromFile(const QString& path) const
{
    lt::error_code ec;
    std::string np = QDir::toNativeSeparators(path).toStdString();
    auto info = boost::shared_ptr<lt::torrent_info>(new lt::torrent_info(np, ec));
    if (ec)
    {
        QString error = QString::fromUtf8(ec.message().c_str());
        LOG << "Cannot load .torrent file: " << error << " path = " << path;
        return 0;
    }

    return info;
}
