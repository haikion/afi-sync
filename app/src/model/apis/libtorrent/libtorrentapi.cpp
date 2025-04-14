#include "libtorrentapi.h"

#include <chrono>
#include <fstream>
#include <memory>

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
#include <libtorrent/version.hpp>
#if LIBTORRENT_VERSION_MAJOR >= 2 && LIBTORRENT_VERSION_MINOR >= 0 && LIBTORRENT_VERSION_TINY >= 11
#define TRUNCATE_SUPPORTED
#include <libtorrent/truncate.hpp>
#endif

#ifdef Q_OS_WINDOWS
#pragma warning(pop)
#endif

#include "pbochecker.h"

#include "model/afisynclogger.h"
#include "model/fileutils.h"
#include "model/global.h"
#include "model/settingsmodel.h"
#include "model/version.h"
#include "model/pbocheckerprinterboost.h"

using namespace Qt::StringLiterals;
using namespace libtorrent;
using namespace std::chrono_literals;

using std::as_const;
using std::ofstream;
using std::shared_ptr;
using std::vector;

constexpr int LibTorrentApi::NOT_FOUND = -404;

LibTorrentApi::LibTorrentApi() :
    QObject()
{
    generalInit();
    QMetaObject::invokeMethod(this, &LibTorrentApi::init, Qt::QueuedConnection);
}

LibTorrentApi::LibTorrentApi(const QStringList& deltaUrls):
    QObject(), // moveToThread cannot be used with parent
    deltaUrls_(deltaUrls)
{
    generalInit();
    if (settings_.deltaPatchingEnabled())
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
    settingsPath_ = settings_.syncSettingsPath() + "/libtorrent.dat";
    moveToThread(Global::workerThread);
}

void LibTorrentApi::generalThreadInit()
{
    using std::make_unique;

    createSession();
    storageMoveManager_ = new StorageMoveManager();
    alertHandler_ = new AlertHandler(this);
    connect(alertHandler_, &AlertHandler::uploadAndDownloadChanged, this, [=, this] (int64_t ul, int64_t dl) {
        uploadSpeed_ = ul;
        downloadSpeed_ = dl;
    });
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &LibTorrentApi::handleAlerts);
    connect(timer_, &QTimer::timeout, storageMoveManager_, &StorageMoveManager::update);
    timer_->setInterval(1s);
    timer_->start();
    networkAccessManager_ = new QNetworkAccessManager(this);
    pboChecker_ = make_unique<PboChecker>();
    pboChecker_->setPrinter(make_unique<PboCheckerPrinterBoost>());
}

LibTorrentApi::~LibTorrentApi()
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    shutdown();
}

void LibTorrentApi::createSession()
{
    settings_pack settings;

    //Disable encryption
    settings.set_int(settings_pack::allowed_enc_level, settings_pack::enc_level::pe_plaintext);
    settings.set_int(settings_pack::in_enc_policy, settings_pack::enc_policy::pe_disabled);
    settings.set_int(settings_pack::out_enc_policy, settings_pack::enc_policy::pe_disabled);
    //Disable uTP, enable TCP
    settings.set_bool(settings_pack::enable_incoming_tcp, true);
    settings.set_bool(settings_pack::enable_outgoing_tcp, true);
    settings.set_bool(settings_pack::enable_incoming_utp, false);
    settings.set_bool(settings_pack::enable_outgoing_utp, false);
    // Disable all peer sources except tracker
    settings.set_bool(settings_pack::enable_dht, false);
    settings.set_bool(settings_pack::enable_lsd, false);

    //Change user agent
    std::string userAgentPrefix = "AFISync/";
    if (Global::guiless)
    {
        userAgentPrefix = "AFISync_Mirror/";
        //Try to maximize upload speed
        settings.set_int(settings_pack::connections_limit, 1000);
        settings.set_int(settings_pack::seed_choking_algorithm, settings_pack::fastest_upload);
    }
    static const std::string versionStr = VERSION_CHARS;
    settings.set_str(settings_pack::user_agent, userAgentPrefix + versionStr);
    //Load port setting
    QString port = settings_.portEnabled() ? settings_.port() : Constants::DEFAULT_PORT;
    settings.set_str(settings_pack::listen_interfaces, "0.0.0.0:" + port.toStdString());
    //Load bandwidth limits
    const QString uLimit = settings_.maxUpload();
    const QString dLimit = settings_.maxDownload();
    if (!uLimit.isEmpty())
    {
        settings.set_int(settings_pack::upload_rate_limit, uLimit.toInt() * 1024);
    }
    if (dLimit.isEmpty())
    {
        settings.set_int(settings_pack::download_rate_limit, dLimit.toInt() * 1024);
    }

    //Sanity check
    if (session_)
    {
        LOG_ERROR << "Session is not null.";
        delete session_;
    }
    session_ = new session(settings);
    loadTorrentFiles(settings_.syncSettingsPath());
}

bool LibTorrentApi::checkFolder(const QString& key)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    Q_ASSERT(pboChecker_);

    LOG << "Checking torrent with key " << key;
    if (deltaManager_ && deltaManager_->contains(key))
    {
        LOG_ERROR << "Torrent is being delta patched. Recheck refused.";
        return false;
    }
    torrent_handle h = getHandle(key);
    if (!h.is_valid())
    {
        return false;
    }
    const auto modPath = SettingsModel::instance().modDownloadPath() + '/' + QString::fromStdString(h.status().name);
    setFolderPaused(h, true);
    checkingPbos_.insert(key);
    truncateOvergrownFiles(h);
    pboChecker_->checkModAsync(modPath, [=, this] (bool success, const QStringList& corruptedPbos) {
        FileUtils::safeRemoveAll(corruptedPbos);
        setFolderPaused(h, false);
        h.force_recheck();
        checkingPbos_.remove(key);
    });
    return true;
}

QStringList LibTorrentApi::folderKeys()
{
    return keyHash_.keys();
}

bool LibTorrentApi::folderNoPeers(const QString& key)
{
    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        return true;
    }

    torrent_status status = handle.status();
    return status.num_peers == 0 && (status.state == torrent_status::state_t::downloading
            || status.state == torrent_status::state_t::downloading_metadata);
}

bool LibTorrentApi::folderReady(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return false;
    }

    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        return false;
    }

    torrent_status status = handle.status();
    // is_finished reports incorrent value
    return status.progress == 1;
}

bool LibTorrentApi::folderChecking(const torrent_status& status)
{
    torrent_status::state_t state = status.state;
    return (state == torrent_status::state_t::checking_files
            || state == torrent_status::state_t::checking_resume_data);
}

bool LibTorrentApi::folderChecking(const QString& key)
{
    if (checkingPbos_.contains(key))
    {
        return true;
    }
    torrent_handle handle = getHandleSilent(key);
    if (!handle.is_valid())
    {
        return false;
    }

    torrent_status status = handle.status();
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
    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        return false;
    }

    torrent_status status = handle.status();
    return status.state == torrent_status::state_t::downloading ||
            status.state == torrent_status::downloading_metadata;
}

bool LibTorrentApi::folderMovingFiles(const QString& key)
{
    return storageMoveManager_->contains(key);
}

bool LibTorrentApi::folderQueued(const torrent_status& status)
{
    //Torrent is always paused when queued
    if (status.is_finished || !(status.flags & torrent_flags::auto_managed))
    {
        return false;
    }

    bool checkingQueued = folderChecking(status) && status.queue_position > 0; //Torrent is paused+checking
    bool downloadQueued = status.state == torrent_status::downloading && status.queue_position > 2;
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

    torrent_handle handle = keyHash_.value(key);
    torrent_status status = handle.status();
    return folderQueued(status);
}

void LibTorrentApi::setDeltaUrls(const QStringList& urls)
{
    QMetaObject::invokeMethod(this, [=, this]
    {
        setDeltaUrlsPriv(urls);
    }, Qt::QueuedConnection);
}

/**
 * @brief LibTorrentApi::setFolderPath
 * @param key
 * @param path
 * @param overwrite True if user chose to overwrite existing files
 */
void LibTorrentApi::setFolderPath(const QString& key, const QString& path, bool overwrite)
{
    torrent_handle handle = getHandle(key);
    auto status = handle.status(torrent_handle::query_save_path | torrent_handle::query_name);
    const QString name = QString::fromStdString(status.name);
    const QString savePath = QString::fromStdString(status.save_path);
    // Path can be drive root in which case it ends with "\" for example "C:\"
    const QString fromPath = QDir::fromNativeSeparators(savePath + (savePath.endsWith('\\') ? QString() : u"\\"_s) + name);
    const QString toPath = QDir::fromNativeSeparators(path + (path.endsWith('\\') ? QString() : u"\\"_s) + name);
    const bool isRunning = !(handle.flags() & torrent_flags::paused);
    const bool recheck = !overwrite && isRunning && QFile::exists(toPath);

    if (recheck) {
        handle.unset_flags(torrent_flags::auto_managed);
        handle.pause();
    }

    storageMoveManager_->insert(key, fromPath, toPath, handle.status().total_wanted_done);
    auto flags = overwrite ? move_flags_t::always_replace_files : move_flags_t::dont_replace;
    handle.move_storage(path.toStdString(), flags);

    if (recheck) {
        handle.force_recheck();
        handle.resume();
        handle.set_flags(torrent_flags::auto_managed);
    }
}

void LibTorrentApi::setFolderPaused(const QString& key, bool value)
{
    torrent_handle handle = getHandle(key);
    setFolderPaused(handle, value);
}

void LibTorrentApi::setFolderPaused(const torrent_handle& handle, bool value)
{
    QString name = QString::fromStdString(handle.status().name);
    if (value)
    {
        handle.unset_flags(torrent_flags::auto_managed);
        handle.pause();
    }
    else
    {
        LOG << "Starting torrent " << name;
        handle.resume();
        if (Global::guiless)
        {
            handle.unset_flags(torrent_flags::auto_managed);
            LOG << "Auto management disabled for " << name << " in mirror mode.";
        }
        else
        {
            handle.set_flags(torrent_flags::auto_managed);
        }
    }
}

bool LibTorrentApi::folderCheckingPatches(const QString& key)
{
    return deltaManager_ && deltaManager_->contains(key) && folderChecking(key);
}

QPair<error_code, add_torrent_params> LibTorrentApi::toAddTorrentParams(const QByteArray& torrentData) {
    Q_ASSERT(!torrentData.isEmpty());
    add_torrent_params atp;
    error_code ec;
    auto dataSize = static_cast<int>(torrentData.size());
    auto ptr = new torrent_info(torrentData.constData(), dataSize, ec);
    if (ec.failed())
    {
        delete ptr;
        return {ec, atp};
    }
    atp.ti = shared_ptr<torrent_info>(ptr);

    QFileInfo fi(SettingsModel::instance().modDownloadPath());
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
    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        return;
    }

    handle.queue_position_top();

    handle.unset_flags(torrent_flags::auto_managed);
    handle.resume();
}

int64_t LibTorrentApi::folderFileSize(const QString& key)
{
    return getHandle(key).torrent_file()->total_size();
}

int64_t LibTorrentApi::folderTotalWanted(const QString& key)
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

    torrent_handle handle = getHandle(key);
    torrent_status status = handle.status();
    return status.state == torrent_status::downloading_metadata ? -1 : status.total_wanted;
}

int64_t LibTorrentApi::folderTotalWantedDone(const QString& key)
{
    if (torrentDownloading_.contains(key))
    {
        return -1;
    }
    if (checkingPbos_.contains(key))
    {
        return 0;
    }
    if (folderMovingFiles(key))
    {
        return storageMoveManager_->totalWantedDone(key);
    }
    if (deltaManager_ && deltaManager_->contains(key))
    {
        return deltaManager_->totalWantedDone(key);
    }

    const torrent_status status = getHandle(key).status();
    if (status.state == torrent_status::downloading_metadata)
    {
        return -1;
    }
    // Circumvent bug within libTorrent.
    if (status.total_wanted_done == 0 && folderChecking(status)) {
        auto totalWantedDone = static_cast<float>(status.total_wanted_done);
        return static_cast<int64_t>(status.progress * totalWantedDone);
    }
    return status.total_wanted_done;
}

// Cleans unused torrent, link and resume datas
void LibTorrentApi::cleanUnusedFiles(const QSet<QString>& usedKeys)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);

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

int64_t LibTorrentApi::bytesToCheck(const torrent_status& status) const
{
    if (!session_)
    {
        return NOT_FOUND;
    }

    int64_t tw = status.total_wanted;
    if (tw == 0) { //Not loaded yet.
        return NOT_FOUND;
    }

    int64_t rVal = tw - status.total_done;

    return rVal;
}

//In early states torrent_file is null.
//This function waits for 5s at most for it to get
//initialized.
shared_ptr<const torrent_info> LibTorrentApi::getTorrentFile(
    const torrent_handle& handle)
{
    auto torrentFile = handle.torrent_file();
    for (int i = 0; !torrentFile; ++i)
    {
        QThread::sleep(1);
        torrentFile = handle.torrent_file();
        if (i >= 5)
        {
            return nullptr;
        }
    }
    return torrentFile;
}

QSet<QString> LibTorrentApi::folderFilesUpper(const QString& key)
{
    QSet<QString> rVal;
    torrent_handle handle = keyHash_.value(key);
    if (!handle.is_valid())
    {
        return rVal;
    }

    auto torrentFile = getTorrentFile(handle);
    if (!torrentFile)
    {
        LOG_ERROR << "torrent_file is null";
        return rVal;
    }
    file_storage files = torrentFile->files();
    for (int i = 0; i < files.num_files(); ++i)
    {
        torrent_status status = handle.status(torrent_handle::query_save_path);
        QString value = QString::fromStdString(status.save_path + "/" + files.file_path(i));
        value = QFileInfo(value).absoluteFilePath().toUpper();
        rVal.insert(value);
    }
    return rVal;
}

bool LibTorrentApi::folderExists(const QString& key)
{
    for (const auto& pending : pendingFolder_)
    {
        if (pending.first == key)
        {
            return true;
        }
    }
    if (torrentDownloading_.contains(key) || (deltaManager_ && deltaManager_->contains(key)))
    {
        return true;
    }
    torrent_handle handle = getHandleSilent(key);
    return handle.is_valid();
}

bool LibTorrentApi::folderPaused(const QString& key)
{
    torrent_handle handle = getHandleSilent(key);
    if (!handle.is_valid())
    {
        return false;
    }

    return handle.flags() & torrent_flags::paused;
}

QString LibTorrentApi::folderError(const QString& key)
{
    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        return {};
    }

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
    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        return {};
    }

    torrent_status status = handle.status();
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
    generateResumeData();
    const auto keys = keyHash_.keys();
    for (const QString& key : keys)
    {
        torrent_handle handle = keyHash_.value(key);
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
        const QList<torrent_handle> handles = deltaManager_->handles();
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
    LOG << "libtorrent shutdown";
    emit shutdownCompleted();
}

int64_t LibTorrentApi::upload()
{
    if (!session_)
    {
        return 0;
    }

    return uploadSpeed_;
}

int64_t LibTorrentApi::download() const
{
    if (!session_)
    {
        return 0;
    }

    return downloadSpeed_;
}

void LibTorrentApi::setMaxUpload(int limit)
{
    QMetaObject::invokeMethod(this, [=, this]
    {
        setMaxDownloadPriv(limit);
    }, Qt::QueuedConnection);
}

void LibTorrentApi::setMaxUploadPriv(const int limit)
{
    settings_pack pack;
    pack.set_int(settings_pack::upload_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

void LibTorrentApi::setMaxDownload(int limit)
{
    QMetaObject::invokeMethod(this, [=, this]
    {
        setMaxUploadPriv(limit);
    }, Qt::QueuedConnection);
}

void LibTorrentApi::setMaxDownloadPriv(const int limit)
{
    settings_pack pack;
    pack.set_int(settings_pack::download_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

bool LibTorrentApi::ready()
{
    return ready_;
}

void LibTorrentApi::setPort(int port)
{
    QMetaObject::invokeMethod(this, [=, this]
    {
        setPortPriv(port);
    }, Qt::QueuedConnection);
}

void LibTorrentApi::setPortPriv(int port)
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
    settings_pack pack;
    pack.set_str(settings_pack::listen_interfaces, "0.0.0.0:" + QString::number(port).toStdString());
    session_->apply_settings(pack);
}

void LibTorrentApi::handleAlerts()
{
    if (!session_)
    {
        LOG_ERROR << "session is null.";
        return;
    }

    session_->post_session_stats();
    vector<alert*> alerts;
    session_->pop_alerts(&alerts);
    alertHandler_->handleAlerts(&alerts);
}

QStringList LibTorrentApi::deltaUrls()
{
    return deltaUrls_;
}

void LibTorrentApi::removeFiles(const QString& hashString)
{
    QString filePrefix = SettingsModel::instance().syncSettingsPath() + '/' + hashString;
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
    QMetaObject::invokeMethod(this, [=, this]
    {
        removeFolderPriv(key);
    }, Qt::QueuedConnection);
}

void LibTorrentApi::removeTorrentParams(const QString& key)
{
    torrentParams_.remove(key);
}

bool LibTorrentApi::removeFolderPriv(const QString& key)
{
    LOG << "key = " << key;
    if (!session_)
    {
        return false;
    }

    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        torrent_status sta = handle.status();
        LOG_ERROR << "Torrent is in invalid state. error = "
                  << QString::fromStdString(sta.errc.message())
                  << " state = " << sta.state;
        return false;
    }
    LOG << "Removing " << key << " from sync";
    session_->remove_torrent(handle);
    removeFiles(getHashString(handle));
    keyHash_.remove(key);
    vector<alert*> alerts;
    //Wait for removed alert (5s)
    //50 alert chunks may be received before torrent removed alert.
    for (int i = 0; i < 50; ++i)
    {
        session_->wait_for_alert(100ms);
        session_->pop_alerts(&alerts);
        for (const alert* alert : alerts)
        {
            if (alert->type() == torrent_removed_alert::alert_type)
            {
                LOG << "Removed alert received. Returning...";
                return true;
            }
        }
    }
    return false;
}

void LibTorrentApi::mirrorDeltaPatches()
{
    QMetaObject::invokeMethod(deltaManager_, &DeltaManager::mirrorDeltaPatches, Qt::QueuedConnection);
}

void LibTorrentApi::setDeltaUrlsPriv(const QStringList& deltaUrls)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (deltaUrls == deltaUrls_)
    {
        return;
    }
    deltaUrls_ = deltaUrls;
    enableDeltaUpdatesPriv();
    deltaManager_->setDeltaUrls(deltaUrls);
}

void LibTorrentApi::disableDeltaUpdates()
{
    QMetaObject::invokeMethod(this, &LibTorrentApi::disableDeltaUpdatesSlot, Qt::QueuedConnection);
}

void LibTorrentApi::disableDeltaUpdatesSlot()
{
    if (!deltaManager_)
    {
        return;
    }

    CiHash<QString> hash = deltaManager_->keyHash();
    disableDeltaUpdatesNoTorrents();
    //Enable regular torrent downloads
    const auto keys = hash.keys();
    for (const QString& key : keys)
    {
        addFolder(key, hash.value(key));
    }
}

void LibTorrentApi::disableDeltaUpdatesNoTorrents()
{
    LOG;
    delete deltaManager_;
    deltaManager_ = nullptr;
}

void LibTorrentApi::enableDeltaUpdates()
{
    QMetaObject::invokeMethod(this, &LibTorrentApi::enableDeltaUpdatesPriv, Qt::QueuedConnection);
}

void LibTorrentApi::enableDeltaUpdatesPriv()
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

torrent_handle LibTorrentApi::addFolderFromParams(const QString& key)
{
    auto it = torrentParams_.find(key);
    if (it != torrentParams_.end())
    {
        LOG << "Adding " << key << " to sync";
        error_code ec;
        torrent_handle handle = session_->add_torrent(it.value(), ec);
        if (ec.failed())
        {
            LOG_ERROR << "Unable to read torrent params for " << key << " " << ec.message();
            return torrent_handle{};
        }
        auto status = handle.status();
        if (deltaManager_ && !deltaManager_->contains(key))
        {
            keyHash_.insert(key, handle);
        }
        return handle;
    }

    return {};
}

//Does not wait for torrent-file download to finish.
bool LibTorrentApi::addFolderGenericAsync(const QString& key)
{
    torrent_handle existingHandle = addFolderFromParams(key);
    if (existingHandle.is_valid())
    {
        onFolderAdded(key, existingHandle);
        return true;
    }

    // TODO: Remove? Might be too defensive
    if (!session_)
    {
        LOG_ERROR << "session is null.";
        onFolderAdded(key, torrent_handle());
        return false;
    }

    QFileInfo fi(settings_.modDownloadPath());
    QDir().mkpath(fi.absoluteFilePath());
    if (!fi.isReadable())
    {
        LOG_ERROR << "Parent dir " << fi.absoluteFilePath() << " is not readable.";
        onFolderAdded(key, torrent_handle());
        return false;
    }
    if (folderExists(key))
    {
        LOG_ERROR << "Torrent already added.";
        onFolderAdded(key, torrent_handle());
        return false;
    }

    auto reply = networkAccessManager_->get(QNetworkRequest(QUrl(key)));
    torrentDownloading_.insert(key);
    LOG << "Downloading torrent " << key << " ...";
    connect(reply, &QNetworkReply::finished, this, [=, this] () {
        torrentDownloading_.remove(key);
        if (reply->error() != QNetworkReply::NoError) {
            LOG_ERROR << "Torrent download failed: " << reply->errorString();
            return;
        }
        LOG << key << " downloaded";

        QByteArray torrentData = reply->readAll();
        reply->deleteLater();

        QPair<error_code, add_torrent_params> pair = toAddTorrentParams(torrentData);
        if (pair.first.failed())
        {
            LOG_WARNING << "Could not convert downloaded data to torrent params: "
                        << pair.first.message();
            return;
        }
        auto atp = pair.second;
        const auto modPath = SettingsModel::instance().modDownloadPath() + '/' + QString::fromStdString(atp.ti->name());
        error_code ec;
        LOG << "Adding " << key << " to sync";
        torrent_handle handle = session_->add_torrent(atp, ec);
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
torrent_handle LibTorrentApi::getHandleSilent(const QString& key)
{
    auto it = keyHash_.find(key);
    if (it != keyHash_.end())
    {
        return it.value();
    }

    if (deltaManager_ && deltaManager_->contains(key))
    {
        return deltaManager_->getHandle(key);
    }

    return {};
}

torrent_handle LibTorrentApi::getHandle(const QString& key)
{
    torrent_handle rVal = getHandleSilent(key);
    if (!rVal.is_valid())
    {
        LOG_ERROR << "Key "  << key << " not found";
    }

    return rVal;
}

bool LibTorrentApi::addFolder(const QString& key, const QString& name)
{
    Q_ASSERT(QThread::currentThread() == Global::workerThread);
    if (!deltaManager_ && !deltaUrls_.isEmpty() && settings_.deltaPatchingEnabled())
    {
        // TODO: Unreachable code block?
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
    if (settings_.deltaPatchingEnabled() &&
            patchingEnabled && deltaManager_ && deltaManager_->patchAvailable(name))
    {
        deltaManager_->patch(name, key);
        return false;
    }
    return addFolderGenericAsync(key);
    // continues at onFolderAdded
}

void LibTorrentApi::onFolderAdded(const QString& key, const torrent_handle& handle) {
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

QString LibTorrentApi::getHashString(const torrent_handle& handle)
{
    torrent_status status = handle.status();
    auto sha1Hash = status.info_hashes.v1;
    auto hex = QByteArray::fromRawData(sha1Hash.data(), sha1Hash.size()).toHex();
    auto hash = QString::fromLatin1(hex);
    return hash;
}

void LibTorrentApi::saveTorrentFile(const torrent_handle& handle) const
{
    using std::back_inserter;

    QString filePrefix = settings_.syncSettingsPath() + "/" + getHashString(handle);
    //Torrent file
    auto ti = getTorrentFile(handle);
    if (!ti)
    {
        LOG_ERROR << "Unable to save torrent file. It is null.";
        return;
    }
    if (!ti->is_valid())
    {
        LOG_ERROR << "Unable to save torrent file. It is invalid.";
        return;
    }
    create_torrent new_torrent(*ti);
    QByteArray torrentBytes;
    bencode(back_inserter(torrentBytes), new_torrent.generate());
    QString torrentFilePath = filePrefix + ".torrent";
    QString urlFilePath = filePrefix + ".link";
    QByteArray url = keyHash_.key(handle).toUtf8();
    if (url.isEmpty())
    {
        url = deltaManager_->getUrl(handle).toUtf8();
    }
    FileUtils::writeFile(torrentBytes, torrentFilePath) && FileUtils::writeFile(url, urlFilePath);
    LOG << handle.status(torrent_handle::query_name).name << " resume data saved";
}

void LibTorrentApi::generateResumeData() const
{
    using std::ios_base;
    using std::ostream_iterator;

    if (!session_)
    {
        LOG_ERROR << "session is null!";
        return;
    }

    int outstanding_resume_data = 0;
    vector<torrent_handle> handles = session_->get_torrents();
    session_->pause();
    for (auto& h : handles)
    {
        if (!h.is_valid())
        {
            continue;
        }
        torrent_status s = h.status();
        if (!s.has_metadata || !s.need_save_resume)
        {
            continue;
        }

        h.save_resume_data();
        ++outstanding_resume_data;
    }

    while (outstanding_resume_data > 0)
    {
        alert* test = session_->wait_for_alert(10s);

        // if we don't get an alert within 10 seconds, abort
        if (test == nullptr)
        {
            break;
        }

        vector<alert*> alerts;
        session_->pop_alerts(&alerts);

        for (alert* a : alerts)
        {
            if (alert_cast<save_resume_data_failed_alert>(a))
            {
                LOG << "Failure in saving resume data: " << a->message().c_str();
                --outstanding_resume_data;
                continue;
            }

            auto rd = alert_cast<save_resume_data_alert>(a);
            if (rd == nullptr)
            {
                continue;
            }

            torrent_handle h = rd->handle;
            auto resumeData = write_resume_data(rd->params);
            ofstream out((settings_.syncSettingsPath().toStdString()
                               + "/" + getHashString(h).toStdString() + ".fastresume").c_str(),
                              ios_base::binary);
            out.unsetf(ios_base::skipws);
            bencode(ostream_iterator<char>(out), resumeData);
            --outstanding_resume_data;
            LOG << "Resume data generated for " << h.status().name.c_str();
        }
    }
}

void LibTorrentApi::createDeltaManager(const QStringList& deltaUrls)
{
    if (deltaManager_)
    {
        LOG << "Replacing old deltaManager " << deltaUrls_
            << " with " << deltaUrls;

        for (const auto& url  : as_const(deltaUrls_)) {
            removeFolder(url);
        }
        delete deltaManager_;
        deltaManager_ = nullptr;
    }

    deltaManager_ = new DeltaManager(deltaUrls, session_, this);
    deltaUrls_ = deltaUrls;
    connect(deltaManager_, &DeltaManager::patched, this, &LibTorrentApi::handlePatched);
}

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
        it.next();
        QString filePath = it.filePath();
        if (!filePath.endsWith(".torrent"_L1)) {
            continue;
        }

        QString pathPrefix = filePath.remove(u".torrent"_s);
        // Load resume data
        QByteArray bytes = FileUtils::readFile(pathPrefix + ".fastresume");
        if (bytes.isEmpty()) {
            LOG_WARNING << "Failed to read resume data for " << pathPrefix;
            continue;
        }
        auto bData = bdecode(bytes);
        add_torrent_params params = read_resume_data(bData);

        params.flags |= torrent_flags::auto_managed;
        params.ti = loadFromFile(pathPrefix + ".torrent");
        params.save_path = settings_.modDownloadPath().toStdString();
        //FIXME: Sometimes this becomes off as empty.
        QString url = QString::fromLocal8Bit(FileUtils::readFile(pathPrefix + ".link")).toLower();
        prefixMap_.insert(url, it.fileName().remove(u".torrent"_s));
        if (params.ti == nullptr || url.isEmpty() || !params.ti->is_valid())
        {
            LOG_ERROR << "Error loading torrent " << filePath << " url = " << url;
            continue;
        }
        QString name = QString::fromStdString(params.ti->name());
        torrentParams_.insert(url, params); // Used in addFolder()
        LOG << "Loaded " << name << " from " << pathPrefix << ".torrent url: " << url;
    }
}

shared_ptr<torrent_info> LibTorrentApi::loadFromFile(const QString& path)
{
    error_code ec;
    std::string np = QDir::toNativeSeparators(path).toStdString();
    auto info = make_shared<torrent_info>(np, ec);
    if (ec)
    {
        QString error = QString::fromUtf8(ec.message().c_str());
        LOG << "Cannot load .torrent file: " << error << " path = " << path;
        return nullptr;
    }

    return info;
}

/**
 * @brief LibTorrentApi::truncateOvergrownFiles
 * @param handle
 * This function is called when a torrent is being checked.
 * It truncates files to their correct size which will not be
 * done by force_recheck().
 * See: https://github.com/qbittorrent/qBittorrent/issues/9782
 */
void LibTorrentApi::truncateOvergrownFiles(const torrent_handle& handle)
{
    shared_ptr<const torrent_info> torrentFile = getTorrentFile(handle);
    if (!torrentFile)
    {
        LOG_ERROR << "Unable to get torrentFile";
        return;
    }
    const file_storage& fs = handle.torrent_file()->files();
    torrent_status status = handle.status(torrent_handle::query_save_path);
    storage_error ec;
#ifdef TRUNCATE_SUPPORTED
    truncate_files(fs, status.save_path, ec);
    if (ec)
    {
        LOG_ERROR << "Truncate failed: " << ec.ec.message();
    }
#else
    LOG_ERROR << "Truncate not supported on this platform.";
#endif
}

void LibTorrentApi::truncateOvergrownFiles(const QString& key)
{
    torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
    {
        LOG_ERROR << "Invalid handle";
        return;
    }
    truncateOvergrownFiles(handle);
}
