#include <vector>
#include <string>

#include <QRegExp>
#include <QDir>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/file_pool.hpp>

#include "../../debug.h"
#include "../../settingsmodel.h"
#include "deltadownloader.h"
#include "ahasher.h"
#include "deltapatcher.h"

namespace lt = libtorrent;

DeltaDownloader::DeltaDownloader(const libtorrent::torrent_handle& handle, QObject* parent):
    QObject(parent),
    handle_(handle)
{
    boost::shared_ptr<const lt::torrent_info> torrent = handle_.torrent_file();
    if (!torrent || !torrent->is_valid())
    {
        DBG << "ERROR: Torrent is invalid.";
        return;
    }
    //Always seed.
    handle_.auto_managed(false);
    handle_.pause();
    fileStorage_ = torrent->files();
    createFilePaths();
    handle_.force_recheck();
    for (int i = 0; i < fileStorage_.num_files(); ++i)
    {
        DBG << handle_.file_priority(i);
        //Do not download anything.
        handle_.file_priority(i, 1); //FixMe: Setting this to 0 causes 0% completion.
    }
}

void DeltaDownloader::createFilePaths()
{
    DBG << fileStorage_.paths().size();
    for (int i = 0; i < fileStorage_.num_files(); ++i)
    {
        //path is sometimes "".
        QString name = QString::fromStdString(fileStorage_.file_name(i));
        QString pathQ = QDir::fromNativeSeparators(name);
        DBG << "Appending" << pathQ;
        filePaths_.append(pathQ);
    }
}

bool DeltaDownloader::patchAvailable(const QString& modName)
{
    fileIndexCache_.remove(modName); //clear cache
    //Check if there is anything to download.
    if (fileIndexes(modName).size() > 0)
        return true;

    return false;
}

bool DeltaDownloader::patchDownloaded(const QString& modName) const
{
    boost::int64_t done = totalWantedDone(modName);
    boost::int64_t wanted = totalWanted(modName);
    DBG << "done =" << done << "/" << wanted;
    return done >= wanted;
}

bool DeltaDownloader::downloadPatch(const QString& modName)
{
    handle_.resume();
    QVector<int> indexes = fileIndexes(modName);
    for (int i : indexes)
    {
        handle_.file_priority(i, 4);
    }
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

boost::int64_t DeltaDownloader::totalWantedDone(const QString& modName) const
{
    boost::int64_t downloaded = 0;
    std::vector<boost::int64_t> progresses;
    handle_.file_progress(progresses, lt::torrent_handle::piece_granularity);

    for (int i : fileIndexes(modName))
    {
        lt::torrent_status status = handle_.status();
        DBG << progresses.at(i) << handle_.file_priority(i);
        downloaded += progresses.at(i);
    }
    return downloaded;
}

boost::int64_t DeltaDownloader::totalWanted(const QString& modName) const
{
    boost::int64_t bytesWanted = 0;
    for (int i : fileIndexes(modName))
        bytesWanted += fileStorage_.file_size(i);

    return bytesWanted;
}

//Returns list of patch file indexes that are applyable.
QVector<int> DeltaDownloader::fileIndexes(const QString& modName) const
{
    //Try to return cached value
    auto it = fileIndexCache_.find(modName);
    if (it != fileIndexCache_.end())
        return it.value();

    //No cache found. Build one.
    QVector<int> indexes;
    QStringList patchPaths = patches(modName);
    for (int i = 0; i < filePaths_.size(); ++i)
    {
        //Filter patch indexes
        if (patchPaths.contains(filePaths_.at(i)))
            indexes.append(i);

    }
    return indexes;
}

QStringList DeltaDownloader::patches(const QString& modName) const
{
    return DeltaPatcher::filterPatches(
                SettingsModel::modDownloadPath() + "/" + modName, filePaths_);
}

QString DeltaDownloader::hash(const QString& modName) const
{
    QString hash = AHasher::hash(
                SettingsModel::modDownloadPath() + "/" + modName);
    return hash;
}
