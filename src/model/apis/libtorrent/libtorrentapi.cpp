#include <fstream>
#include <map>
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
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include "../../fileutils.h"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "libtorrentapi.h"

namespace lt = libtorrent;

const int LibTorrentApi::NOT_FOUND = -404;
const QString LibTorrentApi::ERROR_KEY_NOT_FOUND = "ERROR: not found. key =";
const QString LibTorrentApi::ERROR_SESSION_NULL = "ERROR: session is null.";

LibTorrentApi::LibTorrentApi(QObject *parent) :
    QObject(parent),
    session_(nullptr),
    deltaManager_(nullptr),
    numResumeData_(0),
    settingsPath_(SettingsModel::syncSettingsPath() + "/libtorrent.dat"),
    checkingSpeed_(20000000)
{
    init();
    emit initCompleted();
}

void LibTorrentApi::init()
{
    createSession();
    connect(&alertTimer_, SIGNAL(timeout()), this, SLOT(handleAlerts()));
    alertTimer_.setInterval(1000);
    alertTimer_.start();
}

LibTorrentApi::~LibTorrentApi()
{
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
    std::map<std::string, lt::entry> map = e.dict();
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
    return status.num_peers == 0;
}

bool LibTorrentApi::folderReady(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
        return false;

    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    return status.is_finished;
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
        return false;

    lt::torrent_handle handle = keyHash_.value(key);
    lt::torrent_status status = handle.status();
    return folderQueued(status);
}

void LibTorrentApi::setFolderPaused(const QString& key, bool value)
{
    lt::torrent_handle handle = getHandle(key);

    //TODO: Might be too defensive
    if (!handle.is_valid())
        return;

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

int LibTorrentApi::folderEta(const QString& key)
{
   lt::torrent_status status = getHandle(key).status();
   if (status.state == lt::torrent_status::state_t::checking_files)
   {
       if (status.queue_position == 0)
       {
           return checkingEta(status);
       }
       return queuedCheckingEta(status);
   }
   else if (status.state == lt::torrent_status::downloading && status.queue_position > 0)
   {
       return queuedDownloadEta(status);
   }
   else if (deltaManager_ && deltaManager_->contains(key))
   {
       return deltaManager_->patchingEta(key);
   }
   return downloadEta(status);
}

int LibTorrentApi::checkingEta(const lt::torrent_status& status)
{
    static QMap<lt::sha1_hash, int64_t> lastBytesMap;
    static QMap<lt::sha1_hash, int64_t> lastTime;

    if (lastBytesMap.contains(status.info_hash))
    {
        int64_t dChecked = status.total_wanted_done - lastBytesMap[status.info_hash];
        int64_t dT = runningTimeS() - lastTime[status.info_hash];
        if (dChecked > 0 && dT > 0)
        {
            static const uint64_t SAMPLE_SIZE = 20;
            static uint64_t averager[SAMPLE_SIZE];
            static uint64_t i = 0;

            //Insert average of last <=10 meassurements
            ++i;
            averager[i % SAMPLE_SIZE] = dChecked / dT;
            checkingSpeed_ = std::accumulate(std::begin(averager), std::end(averager), 0) / std::min(i, SAMPLE_SIZE);
            LOG << "Checking speed = " << checkingSpeed_;
        }
    }
    lastBytesMap[status.info_hash] = status.total_wanted_done;
    lastTime[status.info_hash] = runningTimeS();
    if (checkingSpeed_ == 0)
        return Constants::MAX_ETA;

    return (status.total_wanted - status.total_wanted_done) / checkingSpeed_;
}

int LibTorrentApi::queuedDownloadEta(const lt::torrent_status& status) const
{
    qint64 totalBytesToDownload = status.total_wanted;
    qint64 totalBytesToCheck =  status.total_wanted;
    for (const lt::torrent_handle handle : keyHash_.values())
    {
        lt::torrent_status otherStatus = handle.status();
        if (otherStatus.state == lt::torrent_status::state_t::downloading &&
                otherStatus.queue_position < status.queue_position)
        {
            totalBytesToDownload += otherStatus.total_wanted;
            totalBytesToCheck += otherStatus.total_wanted;
        }
        if (otherStatus.state == lt::torrent_status::state_t::checking_files)
        {
            totalBytesToCheck += otherStatus.total_wanted;
        }
    }
    if (download() == 0 || checkingSpeed_ == 0)
    {
        return Constants::MAX_ETA;
    }

    return (totalBytesToDownload / download()) + (totalBytesToCheck / checkingSpeed_);
}

int LibTorrentApi::queuedCheckingEta(const lt::torrent_status& status) const
{
    //Caching
    static QMap<lt::sha1_hash, std::pair<int, unsigned>> cache;
    if (cache.contains(status.info_hash)
            && (runningTimeS() - cache[status.info_hash].second) < 1 )
    {
        return cache[status.info_hash].first;
    }

    if (checkingSpeed_ == 0)
        return Constants::MAX_ETA;

    int rVal = (status.total_wanted - status.total_wanted_done) / checkingSpeed_;
    for (const lt::torrent_handle& handle : keyHash_.values())
    {
        if (handle.queue_position() == (status.queue_position - 1)
                && handle.status().state == lt::torrent_status::state_t::checking_files)
        {
            rVal += cache.contains(handle.info_hash()) ? cache[handle.info_hash()].first : 0;
            break;
        }
    }
    cache[status.info_hash] = std::pair<int, unsigned>(rVal, runningTimeS());

    return rVal;
}

int LibTorrentApi::downloadEta(const lt::torrent_status& status) const
{
    if (status.download_rate == 0)
    {
        return Constants::MAX_ETA;
    }    

    qint64 totalBytesToCheck = 0;
    for (const lt::torrent_handle handle : keyHash_.values())
    {
        lt::torrent_status otherStatus = handle.status();
        if ( otherStatus.state == lt::torrent_status::state_t::checking_files ||
                otherStatus.state == lt::torrent_status::state_t::queued_for_checking ||
                otherStatus.state == lt::torrent_status::state_t::downloading)
        {
            totalBytesToCheck += status.total_wanted;
        }
    }

    return ((status.total_wanted - status.total_wanted_done) / status.download_rate) + (totalBytesToCheck / checkingSpeed_);
}

bool LibTorrentApi::folderPatching(const QString& key)
{
    return deltaManager_ && deltaManager_->contains(key);
}

bool LibTorrentApi::folderDownloadingPatches(const QString& key)
{
    return deltaManager_ && deltaManager_->patchDownloading(key);
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
    boost::shared_ptr<const lt::torrent_info> torrentFile = handle.torrent_file();
    for (int i = 0; !torrentFile; ++i)
    {
        QThread::sleep(1);
        torrentFile = handle.torrent_file();
        if (i >= 5)
            return 0;
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

    boost::shared_ptr<const lt::torrent_info> torrentFile = getTorrentFile(handle);
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
    bool rVal = status.paused;
    //LOG << "key =" << key << "rVal =" << rVal;
    return rVal;
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

    alertTimer_.stop();
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
        session_->remove_torrent(handle);
    }
    if (deltaManager_)
    {
        saveTorrentFile(deltaManager_->handle());
        delete deltaManager_;
        deltaManager_ = nullptr;
        LOG << "Delta Download torrent saved.";
    }
    LOG << "Settings saved";
    if (session_ && deleteSession)
    {
        //FIXME: Hangs here if downloading metadata.
        delete session_;
        session_ = nullptr;
        LOG << "Session deleted";
        keyHash_.clear();
        LOG << "keyHash_ cleared";
    }
}

qint64 LibTorrentApi::upload()
{
    if (!session_)
        return 0;

    return session_->status().payload_upload_rate;
}

qint64 LibTorrentApi::download() const
{
    if (!session_)
        return 0;

    return session_->status().payload_download_rate;
}

void LibTorrentApi::setMaxUpload(unsigned limit)
{
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::upload_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

void LibTorrentApi::setMaxDownload(unsigned limit)
{
    LOG << "Setting max download to " << limit;
    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::download_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

bool LibTorrentApi::ready()
{
    bool rVal = session_->is_valid();
    return rVal;
}

void LibTorrentApi::setPort(int port)
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

void LibTorrentApi::start()
{
    LOG << "Reloading settings...";
    if (session_)
    {
        LOG_ERROR << "Session is not null. Shutting down...";
        shutdown();
    }
    createSession();
    if (SettingsModel::deltaPatchingEnabled() && deltaUpdatesKey_ != "" && !deltaManager_)
    {
        setDeltaUpdatesFolder(deltaUpdatesKey_);
        LOG << "Delta manager restored.";
    }

    alertTimer_.start();
}

void LibTorrentApi::handleAlerts()
{
    if (!session_)
    {
        LOG << ERROR_SESSION_NULL;
        return;
    }

    alerts_ = new std::vector<lt::alert*>();
    session_->wait_for_alert(lt::milliseconds(900));
    session_->pop_alerts(alerts_);
    for (lt::alert* alert : *alerts_)
    {
        handleAlert(alert);
    }
    delete alerts_;
}

QString LibTorrentApi::deltaUpdatesKey()
{
    return deltaUpdatesKey_;
}

bool LibTorrentApi::removeFolder(const QString& key)
{
    static const QString MSG_DELETING_FILE = "Deleting file:";

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
    QString filePrefix = SettingsModel::syncSettingsPath() + "/" + getHashString(handle);
    session_->remove_torrent(handle);
    keyHash_.remove(key);
    //Delete saved data
    QString urlPath = filePrefix + ".link";
    LOG << MSG_DELETING_FILE << " " << urlPath;
    FileUtils::safeRemove(urlPath);
    QString torrentPath = filePrefix + ".torrent";
    LOG << MSG_DELETING_FILE << " " << torrentPath;
    FileUtils::safeRemove(torrentPath);

    std::vector<lt::alert*>* alerts = new std::vector<lt::alert*>();
    QString fastresumePath = filePrefix + ".fastresume";
    LOG << MSG_DELETING_FILE << fastresumePath;
    FileUtils::safeRemove(fastresumePath);
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
    disableDeltaUpdatesNoTorrents();
    deltaUpdatesKey_ = key;
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

bool LibTorrentApi::enableDeltaUpdates()
{
    LOG;

    if (deltaUpdatesKey_.isEmpty())
    {
        LOG_ERROR << "Unable to enable delta updates: deltaUpdatesKey_ is empty.";
        return false;
    }
    if (deltaManager_)
    {
        LOG << "Delta updates already active, doing nothing.";
        return false;
    }
    //Create new deltaManager_;
    lt::torrent_handle handle = addFolderGeneric(deltaUpdatesKey_);
    if (!handle.is_valid())
    {
        LOG_WARNING << "Torrent is invalid. Delta patching disabled.";
        return false;
    }
    createDeltaManager(handle, deltaUpdatesKey_);
    return true;
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

//Does not wait for torrent-file download to finish.
lt::torrent_handle LibTorrentApi::addFolderGenericAsync(const QString& key)
{
    LOG << "key = " << key;

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
    if (QFileInfo(key).exists())
    {
        atp.ti = loadFromFile(key);
    }
    else
    {
        atp.url = key.toStdString();
    }
    atp.save_path = QDir::toNativeSeparators(fi.absoluteFilePath()).toStdString();
    atp.paused = true;
    LOG << "url = " << QString::fromStdString(atp.url)
        << " save_path = " << QString::fromStdString(atp.save_path);
    lt::error_code ec;
    libtorrent::torrent_handle handle = session_->add_torrent(atp, ec);
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
        LOG << ERROR_KEY_NOT_FOUND << key;

    return rVal;
}

bool LibTorrentApi::addFolder(const QString& key, const QString& name)
{
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
    lt::torrent_handle handle = addFolderGenericAsync(key);
    keyHash_.insert(key, handle);
    handle.resume();

    return true;
}

void LibTorrentApi::handlePatched(const QString& key, const QString& modName, bool success)
{
    Q_UNUSED(success)

    QString path = SettingsModel::modDownloadPath();
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
    boost::shared_ptr<lt::torrent_info const> ti = getTorrentFile(handle);
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
    connect(deltaManager_, SIGNAL(patched(QString, QString, bool)),
            this, SLOT(handlePatched(QString, QString, bool)));
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
        LOG << url << (params.ti == 0) << params.resume_data.size();
        if (params.ti == 0 || url.isEmpty() || !params.ti->is_valid())
        {
            LOG_ERROR << "Loading torrent " << filePath << " url = " << url;
            it.next();
            continue;
        }
        QString name = QString::fromStdString(params.ti->name());
        if (name == Constants::DELTA_PATCHES_NAME)
        {
            if (SettingsModel::deltaPatchingEnabled())
            {
                lt::torrent_handle handle = session_->add_torrent(params);
                LOG << "Loading delta patches torrent.";
                createDeltaManager(handle, url);
            }
        }
        else
        {
            LOG << "Appending " << name;
            lt::torrent_handle handle = session_->add_torrent(params);
            keyHash_.insert(url, handle);
        }

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
    boost::shared_ptr<lt::torrent_info> info = boost::shared_ptr<lt::torrent_info>(new lt::torrent_info(np, ec));
    if (ec)
    {
        QString error = QString::fromUtf8(ec.message().c_str());
        LOG << "Cannot load .torrent file: " << error << " path = " << path;
        return 0;
    }

    return info;
}

void LibTorrentApi::handleTorrentFinishedAlert(const lt::torrent_finished_alert* a)
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Torrent " << name << " has finished.";
}

void LibTorrentApi::handleTorrentCheckAlert(const lt::torrent_checked_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Torrent " << name << " checked";
}

void LibTorrentApi::handleFastresumeRejectedAlert(const lt::fastresume_rejected_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG_WARNING << "Fast resume rejected for torrent: " << name;
}

void LibTorrentApi::handleMetadataReceivedAlert(const lt::metadata_received_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Metadata received for torrent: " << name;
}

void LibTorrentApi::handleMetadataFailedAlert(const libtorrent::metadata_failed_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Metadata failed for torrent: " << name;
}

void LibTorrentApi::handlePortmapErrorAlert(const lt::portmap_error_alert* a) const
{
    LOG << "Port map error alert received: " << a->message().c_str();
}

void LibTorrentApi::handlePortmapAlert(const lt::portmap_alert* a) const
{
    LOG << "Port map alert received. Port: " << a->external_port;
}

void LibTorrentApi::handleAlert(lt::alert* a)
{
    try
    {
        switch (a->type())
        {
            case lt::stats_alert::alert_type:
                break;
            case lt::file_renamed_alert::alert_type:
                break;
            case lt::file_completed_alert::alert_type:
                break;
            case lt::torrent_finished_alert::alert_type:
                handleTorrentFinishedAlert(static_cast<lt::torrent_finished_alert*>(a));
                break;
            case lt::save_resume_data_alert::alert_type:
                break;
            case lt::save_resume_data_failed_alert::alert_type:
                break;
            case lt::storage_moved_alert::alert_type:
                break;
            case lt::storage_moved_failed_alert::alert_type:
                break;
            case lt::torrent_paused_alert::alert_type:
                break;
            case lt::tracker_error_alert::alert_type:
                break;
            case lt::tracker_reply_alert::alert_type:
                break;
            case lt::tracker_warning_alert::alert_type:
                break;
            case lt::fastresume_rejected_alert::alert_type:
                handleFastresumeRejectedAlert(static_cast<lt::fastresume_rejected_alert*>(a));
                break;
            case lt::torrent_checked_alert::alert_type:
                handleTorrentCheckAlert(static_cast<lt::torrent_checked_alert*>(a));
                break;
            case lt::metadata_received_alert::alert_type:
                handleMetadataReceivedAlert(static_cast<lt::metadata_received_alert*>(a));
                break;
            case lt::metadata_failed_alert::alert_type:
                handleMetadataFailedAlert(static_cast<lt::metadata_failed_alert*>(a));
                break;
            case lt::state_update_alert::alert_type:
                break;
            case lt::file_error_alert::alert_type:
                break;
            case lt::add_torrent_alert::alert_type:
                break;
            case lt::torrent_removed_alert::alert_type:
                break;
            case lt::torrent_deleted_alert::alert_type:
                break;
            case lt::torrent_delete_failed_alert::alert_type:
                break;
            case lt::portmap_error_alert::alert_type:
                handlePortmapErrorAlert(static_cast<lt::portmap_error_alert*>(a));
                break;
            case lt::portmap_alert::alert_type:
                handlePortmapAlert(static_cast<lt::portmap_alert*>(a));
                break;
            case lt::portmap_log_alert::alert_type:
                break;
            case lt::peer_blocked_alert::alert_type:
                break;
            case lt::peer_ban_alert::alert_type:
                break;
            case lt::url_seed_alert::alert_type:
            case lt::listen_succeeded_alert::alert_type:
                handleListenSucceededAlert(static_cast<lt::listen_succeeded_alert*>(a));
                break;
            case lt::listen_failed_alert::alert_type:
                handleListenFailedAlert(static_cast<lt::listen_failed_alert*>(a));
                break;
            case lt::external_ip_alert::alert_type:
                break;
        }
    }
    catch (std::exception& exc)
    {
        LOG << "Caught exception in: " << exc.what();
    }
}

void LibTorrentApi::handleListenFailedAlert(const lt::listen_failed_alert* a) const
{
    LOG_WARNING << "Received listen failed alert: " << a->what();
}

void LibTorrentApi::handleListenSucceededAlert(const lt::listen_succeeded_alert* a) const
{
    LOG << "Received listen succeeded alert: " << a->what();
}
