#include <algorithm>
#include <string>
#include <vector>
#include <libtorrent/file_pool.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <QDir>
#include <QRegExp>
#include "../../afisynclogger.h"
#include "../../global.h"
#include "../../settingsmodel.h"
#include "ahasher.h"
#include "deltadownloader.h"
#include "deltapatcher.h"


namespace lt = libtorrent;

DeltaDownloader::DeltaDownloader(const libtorrent::torrent_handle& handle):
    handle_(handle)
{
    auto torrent = handle_.torrent_file();
    if (!torrent || !torrent->is_valid())
    {
        LOG_ERROR << "Torrent is invalid.";
        return;
    }
    //Always seed.
    handle_.auto_managed(false);
    handle_.pause();
    fileStorage_ = torrent->files();
    createFilePaths();
    handle.set_sequential_download(true); //Download patches one by one
    for (int i = 0; i < fileStorage_.num_files(); ++i)
    {
        if (Global::guiless) //Seed everything in mirror mode.
        {
                continue;
        }
        //Do not download anything. Sets priority to 0 whenever file does not exist.
        handle_.file_priority(i, DownloadPriority::NO_DOWNLOAD);
    }
    auto priors = handle_.file_priorities();
    handle_.resume();
}

void DeltaDownloader::createFilePaths()
{
    for (int i = 0; i < fileStorage_.num_files(); ++i)
    {
        //path is sometimes "".
        QString name = QString::fromStdString(fileStorage_.file_name(i));
        QString pathQ = QDir::fromNativeSeparators(name);
        LOG << "Appending " << pathQ;
        patches_.append(pathQ);
    }
}

bool DeltaDownloader::patchAvailable(const QString& modName)
{
    fileIndexCache_.remove(modName); //clear cache
    //Check if there is anything to download.
    if (patchIndexes(modName).size() > 0)
        return true;

    return false;
}

bool DeltaDownloader::patchDownloaded(const QString& modName)
{
    const QVector<int> indexes = patchIndexes(modName);
    const boost::int64_t done = totalWantedDone(indexes);
    const boost::int64_t wanted = totalWanted(indexes);
    return done >= wanted;
}

//Downloads all required patches for modName.
//Downloads patches one by one.
bool DeltaDownloader::downloadPatch(const QString& modName)
{
    QVector<int> indexes = patchIndexes(modName);
    for (int i : indexes)
    {
        handle_.file_priority(i, DownloadPriority::NORMAL);
    }
    handle_.resume();
    return indexes.size() > 0;
}

void DeltaDownloader::setPaused(bool value)
{
    if (value)
    {
        handle_.pause();
    }
    else
    {
        handle_.resume();
    }
}

libtorrent::torrent_handle DeltaDownloader::handle()
{
    return handle_;
}

boost::int64_t DeltaDownloader::totalWantedDone(const QVector<int>& indexes)
{
    boost::int64_t downloaded = 0;
    std::vector<boost::int64_t> progresses;

    handle_.file_progress(progresses, lt::torrent_handle::piece_granularity);
    for (int i : indexes)
        downloaded += progresses.at(i);

    return downloaded;
}

boost::int64_t DeltaDownloader::totalWantedDone(const QString& modName)
{
    const libtorrent::torrent_status status = handle_.status();
    const libtorrent::torrent_status::state_t state = status.state;
    if (state == libtorrent::torrent_status::state_t::checking_files ||
            state == libtorrent::torrent_status::state_t::checking_resume_data)
    {
        return status.total_wanted_done;
    }
    return totalWantedDone(patchIndexes(modName));
}

boost::int64_t DeltaDownloader::totalWanted(const QVector<int>& indexes)
{
    boost::int64_t bytesWanted = 0;
    for (int i : indexes)
        bytesWanted += fileStorage_.file_size(i);

    return bytesWanted;
}

boost::int64_t DeltaDownloader::totalWanted(const QString& modName)
{
    const libtorrent::torrent_status status = handle_.status();
    const libtorrent::torrent_status::state_t state = status.state;
    if (state == libtorrent::torrent_status::state_t::checking_files ||
            state == libtorrent::torrent_status::state_t::checking_resume_data)
    {
        return status.total_wanted;
    }
    return totalWanted(patchIndexes(modName));
}

// Returns list of patch file indexes that are applyable to the mod.
QVector<int> DeltaDownloader::patchIndexes(const QString& modName)
{
    //Try to return cached value
    auto it = fileIndexCache_.find(modName);
    if (it != fileIndexCache_.end())
        return it.value();

    //No cache found. Build one.
    QVector<int> indexes;
    QStringList modPatches = patches(modName);
    for (const QString& patchName : modPatches)
    {
        indexes.append(patches_.indexOf(patchName));
    }
    fileIndexCache_.insert(modName, indexes);
    return indexes;
}

QStringList DeltaDownloader::patches(const QString& modName) const
{
    const QString modPath = SettingsModel::modDownloadPath() + "/" + modName;
    return DeltaPatcher::filterPatches(modPath, patches_);
}

QString DeltaDownloader::hash(const QString& modName) const
{
    QString hash = AHasher::hash(
                SettingsModel::modDownloadPath() + "/" + modName);
    return hash;
}
