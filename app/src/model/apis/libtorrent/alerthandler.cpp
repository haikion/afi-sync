#include <libtorrent/session_stats.hpp>
#include "../../afisynclogger.h"
#include "alerthandler.h"

using namespace libtorrent;

AlertHandler::AlertHandler(QObject *parent) : QObject(parent)
{
    downloadIdx_ = lt::find_metric_idx("net.recv_payload_bytes");
    uploadIdx_ = lt::find_metric_idx("net.sent_payload_bytes");
}

void AlertHandler::handleAlerts(const std::vector<alert*>* alerts)
{
    for (alert* alert : *alerts)
    {
        handleAlert(alert);
    }
}

void AlertHandler::handleAlert(alert* alert)
{
    try
    {
        switch (alert->type())
        {
            case file_renamed_alert::alert_type:
                break;
            case file_completed_alert::alert_type:
                break;
            case torrent_finished_alert::alert_type:
                handleTorrentFinishedAlert(static_cast<torrent_finished_alert*>(alert));
                break;
            case save_resume_data_alert::alert_type:
                break;
            case save_resume_data_failed_alert::alert_type:
                break;
            case session_stats_alert::alert_type:
                handleSessionStatsAlert(static_cast<session_stats_alert*>(alert));
                break;
            case storage_moved_alert::alert_type:
                handleStorageMovedAlert(static_cast<storage_moved_alert*>(alert));
                break;
            case storage_moved_failed_alert::alert_type:
                handleStorageMoveFailedAlert(static_cast<storage_moved_failed_alert*>(alert));
                break;
            case torrent_paused_alert::alert_type:
                break;
            case tracker_error_alert::alert_type:
                break;
            case tracker_reply_alert::alert_type:
                break;
            case tracker_warning_alert::alert_type:
                break;
            case fastresume_rejected_alert::alert_type:
                handleFastresumeRejectedAlert(static_cast<fastresume_rejected_alert*>(alert));
                break;
            case torrent_checked_alert::alert_type:
                handleTorrentCheckAlert(static_cast<torrent_checked_alert*>(alert));
                break;
            case metadata_received_alert::alert_type:
                handleMetadataReceivedAlert(static_cast<metadata_received_alert*>(alert));
                break;
            case metadata_failed_alert::alert_type:
                handleMetadataFailedAlert(static_cast<metadata_failed_alert*>(alert));
                break;
            case state_update_alert::alert_type:
                break;
            case file_error_alert::alert_type:
                break;
            case add_torrent_alert::alert_type:
                break;
            case torrent_removed_alert::alert_type:
                break;
            case torrent_deleted_alert::alert_type:
                break;
            case torrent_delete_failed_alert::alert_type:
                break;
            case portmap_error_alert::alert_type:
                handlePortmapErrorAlert(static_cast<portmap_error_alert*>(alert));
                break;
            case portmap_alert::alert_type:
                handlePortmapAlert(static_cast<portmap_alert*>(alert));
                break;
            case portmap_log_alert::alert_type:
                break;
            case peer_blocked_alert::alert_type:
                break;
            case peer_ban_alert::alert_type:
                break;
            case url_seed_alert::alert_type:
            case listen_succeeded_alert::alert_type:
                handleListenSucceededAlert(static_cast<listen_succeeded_alert*>(alert));
                break;
            case listen_failed_alert::alert_type:
                handleListenFailedAlert(static_cast<listen_failed_alert*>(alert));
                break;
            case external_ip_alert::alert_type:
                break;
        }
    }
    catch (std::exception& exc)
    {
        LOG << "Caught exception in: " << exc.what();
    }
}

void AlertHandler::handleFastresumeRejectedAlert(const fastresume_rejected_alert* alert)
{
    torrent_handle h = alert->handle;
    torrent_status s = h.status(torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG_WARNING << "Fast resume rejected for torrent: " << name;
}

void AlertHandler::handleListenFailedAlert(const listen_failed_alert* alert)
{
    LOG_WARNING << "Received listen failed alert: " << alert->what();
}

void AlertHandler::handleListenSucceededAlert(const listen_succeeded_alert* alert)
{
    LOG << "Received listen succeeded alert: " << alert->what();
}

void AlertHandler::handleMetadataFailedAlert(const libtorrent::metadata_failed_alert* alert)
{
    torrent_handle h = alert->handle;
    torrent_status s = h.status(torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Metadata failed for torrent: " << name;
}

void AlertHandler::handleMetadataReceivedAlert(const metadata_received_alert* alert)
{
    torrent_handle h = alert->handle;
    torrent_status s = h.status(torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Metadata received for torrent: " << name;
}

void AlertHandler::handlePortmapAlert(const portmap_alert* alert)
{
    LOG << "Port map alert received. Port: " << alert->external_port;
}

void AlertHandler::handlePortmapErrorAlert(const portmap_error_alert* alert)
{
    LOG << "Port map error alert received: " << alert->message().c_str();
}

void AlertHandler::handleSessionStatsAlert(const libtorrent::session_stats_alert* alert)
{
    const auto stats = alert->counters();
    const int64_t dl = stats[downloadIdx_];
    const int64_t ul = stats[uploadIdx_];

    auto updated = speedCalculator_.update(dl, ul, alert->timestamp());
    if (updated) {
        emit uploadAndDownloadChanged(speedCalculator_.getUploadSpeed(), speedCalculator_.getDownloadSpeed());
    }
}

void AlertHandler::handleStorageMoveFailedAlert(const storage_moved_failed_alert* alert)
{
    LOG << "Received storage move failed alert: " << alert->what();
    emit storageMoveFailed(alert->handle);
}

void AlertHandler::handleStorageMovedAlert(const storage_moved_alert* alert)
{
    LOG << "Received storage moved alert: " << alert->what();
    emit storageMoved(alert->handle);
}

void AlertHandler::handleTorrentCheckAlert(const torrent_checked_alert* alert)
{
    torrent_handle h = alert->handle;
    torrent_status s = h.status(torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Torrent " << name << " checked";
}

void AlertHandler::handleTorrentFinishedAlert(const torrent_finished_alert* alert)
{
    torrent_handle h = alert->handle;
    torrent_status s = h.status(torrent_handle::query_name);
    QString name = QString::fromStdString(s.name);
    LOG << "Torrent " << name << " has finished.";
}
