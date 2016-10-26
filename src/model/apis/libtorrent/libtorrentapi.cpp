#include <map>
#include <fstream>
#include <libtorrent/alert_manager.hpp>
#include <libtorrent/time.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session_status.hpp>
#include <libtorrent/session_handle.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/lazy_entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <QFile>
#include <QFileInfo>
#include <QDirIterator>
#include "../../settingsmodel.h"
#include "../../global.h"
#include "../../debug.h"
#include "../../fileutils.h"
#include "libtorrentapi.h"

namespace lt = libtorrent;

//At least 100 max etas can be summed without overflow
const int LibTorrentApi::MAX_ETA = std::numeric_limits<int>::max()/100;
const QString LibTorrentApi::SETTINGS_PATH = Constants::SYNC_SETTINGS_PATH + "/libtorrent.dat";
const int LibTorrentApi::NOT_FOUND = -404;
const QString LibTorrentApi::ERROR_KEY_NOT_FOUND = "ERROR: not found. key =";
const QString LibTorrentApi::ERROR_SESSION_NULL = "ERROR: session is null.";

LibTorrentApi::LibTorrentApi(QObject *parent) :
    QObject(parent),
    session_(nullptr),
    deltaManager_(nullptr),
    numResumeData_(0)
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
    DBG;
    lt::settings_pack settings;
    //lt::high_performance_seed(settings);

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
    settings.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:" + SettingsModel::port().toStdString());
    //Load bandwidth limits
    QString uLimit = SettingsModel::maxUpload();
    QString dLimit = SettingsModel::maxDownload();
    if (uLimit != "")
        settings.set_int(lt::settings_pack::upload_rate_limit, uLimit.toInt() * 1024);
    if (dLimit != "")
        settings.set_int(lt::settings_pack::download_rate_limit, dLimit.toInt() * 1024);

    //Sanity check
    if (session_)
    {
        DBG << "ERROR: Session is not null.";
        delete session_;
    }
    session_ = new lt::session(settings);
    bool rVal = loadLtSettings();
    loadTorrentFiles(Constants::SYNC_SETTINGS_PATH);
    return rVal;
}

void LibTorrentApi::checkFolder(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
    {
        DBG << "ERROR: Torrent is being delta patched. Recheck refused.";
        return;
    }
    lt::torrent_handle h = getHandle(key);
    if (!h.is_valid())
        return;

    DBG << "Checking torrent with key" << key;
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
    writeFile(bytes, SETTINGS_PATH);
    DBG << "Data saved." << bytes.size() << "bytes written.";
}

//Loads libTorrent specific settings
bool LibTorrentApi::loadLtSettings()
{
    DBG;
    if (!session_)
        return false;

    QByteArray bytes = FileUtils::readFile(SETTINGS_PATH);
    if (bytes.size() == 0)
        return false;
    lt::bdecode_node fast;
    lt::error_code ec;
    lt::bdecode(bytes.constData(), bytes.constData() + bytes.size(), fast, ec);
    DBG << bytes.size() << "bytes read.";
    if (ec || (fast.type() != lt::bdecode_node::dict_t))
    {
        DBG << "Failure";
        return false;
    }
    session_->load_state(fast);
    DBG << "Data loaded";
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
    if (deltaManager_ && deltaManager_->contains(key.toLower()))
        return false;

    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    return status.is_finished;
}

bool LibTorrentApi::folderChecking(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    lt::torrent_status::state_t state = status.state;
    //DBG << "key =" << key << "state =" << state << "active time =" << handle.name().c_str()
    //    << "\n allocating =" << lt::torrent_status::state_t::allocating
    //    << "\n checking files =" << lt::torrent_status::state_t::checking_files
    //    << "\n checking_resume_data =" << lt::torrent_status::state_t::checking_resume_data
    //    << "\n queued_for_checking =" << lt::torrent_status::state_t::queued_for_checking;
    return (state == lt::torrent_status::state_t::checking_files
         || state == lt::torrent_status::state_t::checking_resume_data
         || state == lt::torrent_status::state_t::allocating);
}

//Overly complicated function which returns true if folder is queued for anything.
//libTorrent does not seem to have a proper way to query this...
bool LibTorrentApi::folderQueued(const QString& key)
{
    if (deltaManager_ && deltaManager_->contains(key))
        return false;

    lt::torrent_handle handle = keyHash_.value(key.toLower());
    lt::torrent_status status = handle.status();
    lt::torrent_status::state_t state = status.state;

    //Torrent is always paused when queued
    if (!status.auto_managed || status.is_finished || !status.paused)
        return false;

    //Default active downloads is 3.
    //DBG << "key =" << key << "state =" << state << "name =" << handle.name().c_str()
    //    << "\n allocating =" << lt::torrent_status::state_t::allocating
    //    << "\n checking files =" << lt::torrent_status::state_t::checking_files
    //    << "\n checking_resume_data =" << lt::torrent_status::state_t::checking_resume_data
    //    << "\n queued_for_checking =" << lt::torrent_status::state_t::queued_for_checking
    //    << "\n pos =" << status.queue_position;
    //Auto manager pauses torrent when queued. status.queue_position cannot be used
    //as activation threshold may change for arbitary reasons.
    bool checkingQueued = folderChecking(key)
            //This state is usually not set when queued for checking...
            || state == lt::torrent_status::state_t::queued_for_checking;
    bool downloadQueued = state == lt::torrent_status::downloading;
    return checkingQueued || downloadQueued;
}

void LibTorrentApi::setFolderPaused(const QString& key, bool value)
{
    lt::torrent_handle handle = getHandle(key);

    if (!handle.is_valid())
        return;

    QString name = QString::fromStdString(handle.status().name);
    if (value)
    {
        DBG << "Pausing torrent" << name;
        handle.auto_managed(false);
        handle.pause();
        DBG << handle.status().paused;
    }
    else
    {
        DBG << "Starting torrent" << name;
        handle.resume();
        if (Global::guiless)
        {
            handle.auto_managed(false);
            DBG << "Auto management disabled for" << name << "in mirror mode.";
        }
        else
        {
            handle.auto_managed(true);
        }
    }
}

int LibTorrentApi::folderEta(const QString& key)
{
    lt::torrent_handle handle;
    QString torrentKey = key;
    int increment = 0;
    if (deltaManager_ && (key == deltaUpdatesKey_ || deltaManager_->contains(key)))
    {
        handle = deltaManager_->handle();
        increment = deltaManager_->patchingEta(key);
        torrentKey = deltaUpdatesKey_;
    }
    else
    {
        handle = getHandle(key);
    }
    if (!handle.is_valid())
    {
        DBG << ERROR_KEY_NOT_FOUND << key;
        return MAX_ETA;
    }

    lt::torrent_status status = handle.status();

    if (folderChecking(torrentKey))
    {
        int64_t bc = bytesToCheck(status);
        if (bc == NOT_FOUND)
            return NOT_FOUND;

        int64_t checkingSpeed;
        if (folderQueued(torrentKey))
        {
            checkingSpeed = speedEstimator_.estimation();
        }
        else
        {
            checkingSpeed = speedEstimator_.estimate(torrentKey, bc);
        }
        int64_t rVal = bc/checkingSpeed;
        return rVal + increment;
    }
    //Cut checking speed estimation.
    speedEstimator_.cutEsimation(torrentKey);
    if (status.is_finished)
        return increment;

    int download = status.download_rate;
    if (download == 0)
        return MAX_ETA;

    if (status.total_wanted == 0)
    {
        DBG << "ERROR: total_wanted = 0";
        return NOT_FOUND;
    }
    boost::int64_t totalWanted = status.total_wanted;
    boost::int64_t totalWantedDone = status.total_wanted_done;
    if (key == deltaUpdatesKey_)
    {
        //In case of delta update, only account for mod specific patch downloads.
        totalWanted = deltaManager_->totalWanted(key);
        totalWantedDone = deltaManager_->totalWantedDone(key);
    }

    return ((totalWanted - totalWantedDone) / download) + increment;
}

bool LibTorrentApi::folderPatching(const QString& key)
{
    return deltaManager_ && deltaManager_->contains(key);
}

int64_t LibTorrentApi::bytesToCheck(const lt::torrent_status& status) const
{
    if (!session_)
        return NOT_FOUND;

    int64_t tw = status.total_wanted;
    if (tw == 0) //Not loaded yet.
        return NOT_FOUND;

    int64_t rVal = tw*(1 - status.progress);
    //DBG << "rVal =" << rVal;
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
    DBG << "key =" << key;
    QSet<QString> rVal;
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return rVal;

    boost::shared_ptr<const lt::torrent_info> torrentFile = getTorrentFile(handle);
    if (!torrentFile)
    {
        DBG << "ERROR: torrent_file is null";
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
    return getHandleSilent(key).is_valid();
}


bool LibTorrentApi::folderPaused(const QString& key)
{
    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    lt::torrent_status status = handle.status();
    bool rVal = status.paused;
    //DBG << "key =" << key << "rVal =" << rVal;
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
    DBG << "key =" << key << "rVal =" << rVal;
    return rVal;
}

//Shutdowns libtorrent session and saves settings.
void LibTorrentApi::shutdown()
{
    alertTimer_.stop();
    DBG << "Alert timer stopped";
    saveSettings();
    for (const lt::torrent_handle& handle : keyHash_.values())
    {
        saveTorrentFile(handle);
    }
    if (deltaManager_)
    {
        saveTorrentFile(deltaManager_->handle());
        delete deltaManager_;
        DBG << "Delta Download torrent saved.";
    }
    generateResumeData();
    DBG << "Settings saved";
    if (session_)
    {
        delete session_;
        session_ = nullptr;
        DBG << "Session deleted";
    }
    keyHash_.clear();
    DBG << "keyHash_ cleared";
}

qint64 LibTorrentApi::upload()
{
    if (!session_)
        return 0;

    return session_->status().payload_upload_rate;
}

qint64 LibTorrentApi::download()
{
    if (!session_)
        return 0;

    return session_->status().payload_download_rate;
}

void LibTorrentApi::setMaxUpload(unsigned limit)
{
    if (!session_)
    {
        DBG << ERROR_SESSION_NULL;
        return;
    }

    lt::settings_pack pack;
    pack.set_int(lt::settings_pack::upload_rate_limit, limit * 1024);
    session_->apply_settings(pack);
}

void LibTorrentApi::setMaxDownload(unsigned limit)
{
    if (!session_)
    {
        DBG << ERROR_SESSION_NULL;
        return;
    }

    DBG << "Setting max download to" << limit;
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
        DBG << "ERROR: session is null";
        return;
    }

    DBG << "port =" << port;
    if (port < 0)
    {
        DBG << "Incorrect port" << port << "given";
        return;
    }
    lt::settings_pack pack;
    pack.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:" + QString::number(port).toStdString());
    session_->apply_settings(pack);
}

void LibTorrentApi::start()
{
    DBG << "Reloading settings...";
    if (session_)
    {
        DBG << "ERROR: Session is not null. Shutting down...";
        shutdown();
    }
    createSession();
    alertTimer_.start();
}

void LibTorrentApi::handleAlerts()
{
    if (!session_)
    {
        DBG << ERROR_SESSION_NULL;
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

    DBG << "key =" << key;
    if (!session_)
        return false;

    lt::torrent_handle handle = getHandle(key);
    if (!handle.is_valid())
        return false;

    if (!handle.is_valid())
    {
        lt::torrent_status sta = handle.status();
        DBG << "ERROR: Torrent is in invalid state. error ="
            << QString::fromStdString(sta.error)
            << "state =" << sta.state;

        return false;
    }
    QString filePrefix = Constants::SYNC_SETTINGS_PATH + "/" + getHashString(handle);
    session_->remove_torrent(handle);
    keyHash_.remove(key);
    //Delete saved data
    QString urlPath = filePrefix + ".link";
    DBG << MSG_DELETING_FILE << urlPath;
    FileUtils::safeRemove(urlPath);
    QString torrentPath = filePrefix + ".torrent";
    DBG << MSG_DELETING_FILE << torrentPath;
    FileUtils::safeRemove(torrentPath);

    std::vector<lt::alert*>* alerts = new std::vector<lt::alert*>();
    QString fastresumePath = filePrefix + ".fastresume";
    DBG << MSG_DELETING_FILE << fastresumePath;
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
                DBG << "Removed alert received. Returning...";
                delete alerts;
                return true;
            }
        }
    }
    delete alerts;
    return false;
}

void LibTorrentApi::setDeltaUpdatesFolder(const QString& key, const QString& path)
{
    DBG << "path =" << path << "key =" << key;
    CiHash<QString> keyHash;
    if (deltaManager_)
    {
        DBG << "Updating delta updates folder...";
        keyHash = deltaManager_->keyHash();
        delete deltaManager_;
        deltaManager_ = nullptr;
    }

    lt::torrent_handle handle = addFolderGeneric(key, path);
    if (!handle.is_valid())
    {
        DBG << "Warning: Torrent is invalid. Delta patching disabled.";
        return;
    }
    createDeltaManager(handle, key);
    for (QString key : keyHash)
    {
        QString name = keyHash.value(key);
        DBG << "Retrying patching for" << name << key;
        //Re-apply pending patches.
        deltaManager_->patch(name, key);
    }
}

lt::torrent_handle LibTorrentApi::addFolderGeneric(const QString& key, const QString path)
{
    DBG << "path =" << path << "key =" << key;

    if (!session_)
    {
        DBG << ERROR_SESSION_NULL;
        return lt::torrent_handle();
    }

    QFileInfo fi(path);
    QDir().mkpath(fi.absoluteFilePath());
    if (!fi.isReadable())
    {
        DBG << "ERROR: Parent dir " << fi.absoluteFilePath() << "is not readable.";
        return lt::torrent_handle();
    }
    QString lowerKey = key.toLower();
    if (folderExists(lowerKey))
    {
        DBG << "ERROR: Torrent already added.";
        return lt::torrent_handle();
    }
    lt::add_torrent_params atp;
    atp.url = lowerKey.toStdString();
    atp.save_path = QDir::toNativeSeparators(fi.absoluteFilePath()).toStdString();
    atp.paused = true;
    DBG << "url =" << QString::fromStdString(atp.url)
        << "save_path =" << QString::fromStdString(atp.save_path);
    lt::error_code ec;
    libtorrent::torrent_handle handle = session_->add_torrent(atp, ec);
    if (ec)
    {
        DBG << "ERROR: Adding torrent failed:" << ec.message().c_str();
        return lt::torrent_handle();
    }
    //Wait for torrent file to download.
    lt::torrent_status status = handle.status();
    for (int a = 0; a < 100 && status.state == lt::torrent_status::downloading_metadata; ++a)
    {
        QThread::msleep(100);
        status = handle.status();
    }
    if (status.state == lt::torrent_status::downloading_metadata)
    {
        DBG << "ERROR: Failure downloading torrent from" << key << ":" << status.error.c_str();
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
        DBG << ERROR_KEY_NOT_FOUND << key;

    return rVal;
}

bool LibTorrentApi::addFolder(const QString& key, const QString& path, const QString& name)
{
    return addFolder(key, path, name, true);
}

bool LibTorrentApi::addFolder(const QString& key, const QString& path, const QString& name, bool patchingEnabled)
{
    QString lowerKey = key.toLower();

    if (SettingsModel::deltaPatchingEnabled() &&
            patchingEnabled && deltaManager_ && deltaManager_->patchAvailable(name))
    {
        deltaManager_->patch(name, lowerKey);
        return true;
    }
    lt::torrent_handle handle = addFolderGeneric(lowerKey, path);
    keyHash_.insert(lowerKey, handle);
    handle.resume();

    return true;
}

void LibTorrentApi::handlePatched(const QString& key, const QString& modName, bool success)
{
    Q_UNUSED(success)

    QString path = SettingsModel::modDownloadPath();
    DBG << "Patching done. Readding mod." << key << modName << success;
    //Re-add folder and prevent infinite loop by patch refusal.
    addFolder(key, SettingsModel::modDownloadPath(), modName, false);
}

QString LibTorrentApi::getHashString(const lt::torrent_handle& handle) const
{
    lt::torrent_status status = handle.status();
    char out[(libtorrent::sha1_hash::size * 2) + 1];
    lt::to_hex((char const*)&status.info_hash[0], libtorrent::sha1_hash::size, out);
    QString hash = QString(out);
    DBG << "hash =" << hash << "name =" << QString::fromStdString(status.name);
    return hash;
}

bool LibTorrentApi::saveTorrentFile(const lt::torrent_handle& handle) const
{
    QString filePrefix = Constants::SYNC_SETTINGS_PATH + "/" + getHashString(handle);
    //Torrent file
    boost::shared_ptr<lt::torrent_info const> ti = getTorrentFile(handle);
    DBG << "name =" << QString::fromStdString(handle.status(lt::torrent_handle::query_name).name);
    if (!ti)
    {
        DBG << "ERROR: Unable to save torrent file. It is null.";
        return false;
    }
    if (!ti->is_valid())
    {
        DBG << "ERROR: Unable to save torrent file. It is invalid.";
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

    return writeFile(torrentBytes, torrentFilePath) && writeFile(url, urlFilePath);
}

void LibTorrentApi::generateResumeData() const
{
    DBG;
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
                DBG << "Failure in saving resume data:" << a->message().c_str();
                --outstanding_resume_data;
                continue;
            }

            lt::save_resume_data_alert const* rd = lt::alert_cast<lt::save_resume_data_alert>(a);
            if (rd == 0)
                continue;

            lt::torrent_handle h = rd->handle;

            std::ofstream out((Constants::SYNC_SETTINGS_PATH.toStdString()
                               + "/" + getHashString(h).toStdString() + ".fastresume").c_str()
                    , std::ios_base::binary);
            out.unsetf(std::ios_base::skipws);
            bencode(std::ostream_iterator<char>(out), *rd->resume_data);
            --outstanding_resume_data;
            DBG << "Resume data generated for" << h.status().name.c_str();
        }
        DBG << "Deleting alerts";
        delete alerts;
    }
}

//Writes a new file
bool LibTorrentApi::writeFile(const QByteArray& data, const QString& path) const
{
    QFile file(path);
    QDir parentDir = QFileInfo(path).dir();
    parentDir.mkpath(".");
    FileUtils::safeRemove(file);
    file.open(QFile::WriteOnly);

    if (!file.isWritable())
    {
        DBG << "ERROR: file" << path << " is not writable.";
        return false;
    }
    DBG << "Writing file:" << path;
    file.write(data);
    file.close();
    return true;
}

void LibTorrentApi::createDeltaManager(lt::torrent_handle handle, const QString& key)
{
    deltaManager_ = new DeltaManager(handle, this);
    deltaUpdatesKey_ = key.toLower();
    connect(deltaManager_, SIGNAL(patched(QString, QString, bool)),
            this, SLOT(handlePatched(QString, QString, bool)));
}

void LibTorrentApi::loadTorrentFiles(const QDir& dir)
{
    if (!dir.isReadable())
    {
        DBG << "Warning: directory" << dir.absolutePath() << "is not readable.";
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
        DBG << "Processing:" << filePath;

        QString pathPrefix = filePath.remove(".torrent");
        lt::add_torrent_params params;
        params.ti = loadFromFile(pathPrefix + ".torrent");
        params.save_path = SettingsModel::modDownloadPath().toStdString();
        params.resume_data = loadResumeData(pathPrefix + ".fastresume");
        QString url = QString::fromLocal8Bit(FileUtils::readFile(pathPrefix + ".link"));
        DBG << url << (params.ti == 0) << params.resume_data.size();
        if (params.ti == 0 || url.isEmpty() || !params.ti->is_valid())
        {
            DBG << "ERROR: loading torrent" << filePath
                << "url =" << url;
            it.next();
            continue;
        }
        lt::torrent_handle handle = session_->add_torrent(params);
        QString name = QString::fromStdString(params.ti->name());
        if (name == Constants::DELTA_PATCHES_NAME)
        {
            if (SettingsModel::deltaPatchingEnabled())
            {
                DBG << "Loading delta patches torrent.";
                createDeltaManager(handle, url);
            }
        }
        else
        {
            DBG << "Appending" << name;
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
        DBG << "Cannot load .torrent file:" << error << "path =" << path;
        return 0;
    }

    return info;
}

void LibTorrentApi::handleTorrentFinishedAlert(const lt::torrent_finished_alert* a)
{

    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    DBG << "Torrent" << name << "has finished.";
}

void LibTorrentApi::handleTorrentCheckAlert(const lt::torrent_checked_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    DBG << "Torrent" << name << "checked";
}

void LibTorrentApi::handleFastresumeRejectedAlert(const lt::fastresume_rejected_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    DBG << "Warning: Fast resume rejected for torrent:" << name;
}

void LibTorrentApi::handleMetadataReceivedAlert(const lt::metadata_received_alert* a) const
{
    lt::torrent_handle h = a->handle;
    lt::torrent_status s = h.status(lt::torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    DBG << "Metadata received for torrent:" << name;
}

void LibTorrentApi::handlePortmapErrorAlert(const lt::portmap_error_alert* a) const
{
    DBG << "Port map error alert received:" << a->message().c_str();
}

void LibTorrentApi::handlePortmapAlert(const lt::portmap_alert* a) const
{
    DBG << "Port map alert received. Port:" << a->external_port;
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
        DBG << "Caught exception in:" << exc.what();
    }
}

void LibTorrentApi::handleListenFailedAlert(const lt::listen_failed_alert* a) const
{
    DBG << "Warning: Received listen failed alert:" << a->what();
}

void LibTorrentApi::handleListenSucceededAlert(const lt::listen_succeeded_alert* a) const
{
    DBG << "Received listen succeeded alert:" << a->what();
}
