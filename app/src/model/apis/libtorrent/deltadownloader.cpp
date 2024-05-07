#include "libtorrentapi.h"

#include <libtorrent/file_storage.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>

#include <QDir>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "../../settingsmodel.h"
#include "src/model/afisync.h"
#include "ahasher.h"
#include "deltadownloader.h"
#include "src/model/afisynclogger.h"

using namespace Qt::StringLiterals;

namespace lt = libtorrent;

DeltaDownloader::DeltaDownloader():
    QObject()
{}

void DeltaDownloader::mirrorDeltaPatches()
{ 
    QStringList urls = deltaUrls_;
    inDl_ = urls;
    for (const QString& url : urls) {
        if (urlHandleMap_.contains(url))
        {
            continue;
        }
        mirrored_.append(url);
        auto reply = nam_.get(QNetworkRequest(QUrl(url)));
        LOG << "Downloading " << url << " ...";
        connect(reply, &QNetworkReply::finished, this, [=] ()
        {
            if (reply->error() != QNetworkReply::NetworkError::NoError) {
                LOG_WARNING << "Torrent download failed: " << reply->errorString();
                return;
            }
            auto bytes = reply->readAll();
            QPair<lt::error_code, lt::add_torrent_params> pair = LibTorrentApi::toAddTorrentParams(bytes);
            if (pair.first.failed())
            {
                LOG_WARNING << "Could not convert " << url <<  " to torrrent params: "
                            << pair.first.message();
                return;
            }
            lt::add_torrent_params atp = pair.second;
            atp.save_path = SettingsModel::patchesDownloadPath().toStdString();
            lt::error_code ec;
            auto handle = session_->add_torrent(atp, ec);
            if (ec.failed())
            {
                LOG_WARNING << "Adding torrent failed: " << ec.message().c_str();
                return;
            }
            LOG << "Downloaded " << url;
            urlHandleMap_.insert(url, handle);
        });
    }
}

bool DeltaDownloader::patchAvailable(const QString& modName)
{
    for (const auto& url : deltaUrls_) {
        if (url.contains(modName, Qt::CaseInsensitive)) {
            auto modHash = hash(modName);
            return url.contains(modHash);
        }
    }
    return false;
}

// Downloads all required patches for modName.
bool DeltaDownloader::downloadPatches(const QString& modName, const QString& key)
{
    QStringList urls = getDeltaUrls(modName);
    const auto urlsCopy = urls;
    for (const QString& url: urlsCopy)
    {
        if (mirrored_.contains(url))
        {
            auto it = urlHandleMap_.find(url);
            if (it == urlHandleMap_.end())
            {
                // Cancel add
                mirrored_.removeOne(url);
                continue;
            } 
            urls.removeOne(url);
            mirrored_.removeOne(url);
            addToHandleMap(key, *it);
        }
    }
    inDl_ = urls;
    for (const QString& url : urls) {
        auto reply = nam_.get(QNetworkRequest(QUrl(url)));
        LOG << "Downloading " << url << " ...";
        connect(reply, &QNetworkReply::finished, this, [=] ()
        {
            inDl_.removeOne(url);
            if (reply->error() != QNetworkReply::NetworkError::NoError) {
                LOG_WARNING << "Unable to download torrent: " << reply->errorString();
                emit patchDlFailed();
                return;
            }
            auto bytes = reply->readAll();
            auto pair = LibTorrentApi::toAddTorrentParams(bytes);
            if (pair.first.failed())
            {
                LOG_WARNING << "Could not convert " << url <<  " to torrrent params: "
                            << pair.first.message();
                emit patchDlFailed();
                return;
            }
            auto atp = pair.second;
            atp.save_path = SettingsModel::patchesDownloadPath().toStdString();
            lt::error_code ec;
            auto handle = session_->add_torrent(atp, ec);
            if (ec.failed())
            {
                LOG_WARNING << "Adding torrent failed: " << ec.message().c_str();
                emit patchDlFailed();
                return;
            }
            LOG << "Downloaded " << url;
            addToHandleMap(key, handle);
            urlHandleMap_.insert(url, handle);
        });
    }
    return true;
}

bool DeltaDownloader::patchDownloading(const QString& modName) const
{
    auto it = handleMap_.constFind(modName);
    if (it == handleMap_.end())
    {
        return false;
    }

    const auto handles = *it;
    for (const auto& handle : handles) {
        if (!handle.status().is_finished) {
            return true;
        }
    }
    return false;
}

bool DeltaDownloader::patchesDownloaded(const QString& key) const
{
    if (!inDl_.isEmpty())
    {
        return false;
    }
    auto it = handleMap_.constFind(key);
    if (it == handleMap_.end())
    {
        return false;
    }

    const auto handles = *it;
    for (const auto& handle : handles) {
        if (!handle.status().is_finished) {
            LOG << "Still downloading ...";
            return false;
        }
    }
    return true;

}

void DeltaDownloader::setDeltaUrls(const QStringList& deltaUrls)
{
    deltaUrls_ = deltaUrls;
}

QStringList DeltaDownloader::deltaUrls() const
{
    return deltaUrls_;
}

const QList<libtorrent::torrent_handle> DeltaDownloader::handles() const
{
    return urlHandleMap_.values();
}

QString DeltaDownloader::getUrl(const libtorrent::torrent_handle& handle) const
{
    return urlHandleMap_.key(handle);
}

void DeltaDownloader::setSession(lt::session* newSession)
{
    session_ = newSession;
}

void DeltaDownloader::addToHandleMap(const QString& key, const libtorrent::torrent_handle& torrentHandle)
{
    auto it = handleMap_.find(key);
    if (it != handleMap_.end()) {
        it->append(torrentHandle);
    } else {
        handleMap_.insert(key, {torrentHandle});
    }
}

QStringList DeltaDownloader::getDeltaUrls(const QString& modName) const
{
    auto modHash = hash(modName);
    return AfiSync::getDeltaUrlsForMod(modName, modHash, deltaUrls_);
}

libtorrent::torrent_handle DeltaDownloader::getHandle(const QString& key) const
{
    auto it = handleMap_.find(key);
    return it == handleMap_.end() ? lt::torrent_handle{} : it->at(0);
}

int64_t DeltaDownloader::totalWantedDone(const QString& modName)
{
    const QList<lt::torrent_handle> torrentList = handleMap_.value(modName);
    int64_t total = 0;
    for (const auto& torrent : torrentList) {
        total += torrent.status().total_wanted_done;
    }
    return total;
}

boost::int64_t DeltaDownloader::totalWanted(const QString& modName)
{
    int64_t totalWanted = 0;
    const auto torrentList = handleMap_.value(modName);
    for (const auto& torrent : torrentList) {
        totalWanted += torrent.status().total_wanted;
    }
    return totalWanted;
}

QString DeltaDownloader::hash(const QString& modName)
{
    QString hash = AHasher::hash(
                SettingsModel::modDownloadPath() + "/" + modName);
    return hash;
}
