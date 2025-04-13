#include "alerthandler.h"

#include <libtorrent/alert_types.hpp>
#include <libtorrent/session_stats.hpp>

#include "../../afisynclogger.h"

using namespace libtorrent;
namespace {
    std::string getName(alert* alert)
    {
        auto torrentAlert = dynamic_cast<torrent_alert*>(alert);
        if (!torrentAlert)
        {
            return {};
        }
        torrent_handle h = torrentAlert->handle;
        torrent_status s = h.status(torrent_handle::query_name);
        return s.name;
    }
}


AlertHandler::AlertHandler(QObject* parent)
    : QObject(parent),
    downloadIdx_(find_metric_idx("net.recv_payload_bytes")),
    uploadIdx_(find_metric_idx("net.sent_payload_bytes"))
{
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
            case fastresume_rejected_alert::alert_type:
                LOG_WARNING << "Fast resume rejected for torrent: " << getName(alert);
                break;
            case listen_failed_alert::alert_type:
                LOG_WARNING << "Received listen failed alert: " << alert->what();
                break;
            case listen_succeeded_alert::alert_type:
                LOG << "Received listen succeeded alert: " << alert->what();
                break;
            case metadata_failed_alert::alert_type:
                LOG << "Metadata failed for torrent: " << getName(alert);
                break;
            case metadata_received_alert::alert_type:
                LOG << "Metadata received for torrent: " << getName(alert);
                break;
            case peer_ban_alert::alert_type:
                LOG_WARNING << "Received peer ban alert for "
                            << getName(alert) << ": " << alert->what();
                break;
            case peer_connect_alert::alert_type:
                LOG << "Peer connect alert received for torrent: " << getName(alert);
                break;
            case portmap_alert::alert_type:
                handlePortmapAlert(static_cast<portmap_alert*>(alert));
                break;
            case portmap_error_alert::alert_type:
                LOG << "Port map error alert received: " << alert->message().c_str();
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
            case torrent_checked_alert::alert_type:
                LOG << "Torrent " << getName(alert) << " checked";
                break;
            case torrent_finished_alert::alert_type:
                LOG << "Torrent " << getName(alert) << " has finished.";
                break;
            case tracker_error_alert::alert_type:
                LOG_WARNING << "Tracker error: " << alert->message();
                break;
            case url_seed_alert::alert_type:
                LOG << "URL seed alert: " << alert->message();
                break;
            case tracker_reply_alert::alert_type:
                LOG << "Tracker reply: " << alert->message();
                break;
            case tracker_announce_alert::alert_type:
                LOG << "Tracker announce: " << alert->message();
                break;
            case incoming_connection_alert::alert_type:
                LOG << "Incoming connection: " << alert->message();
                break;
            default:
                // LOG << "Unhandled alert: " << alert->what() << " " << alert->message();
                break;
        }
    }
    catch (std::exception& exc)
    {
        LOG_WARNING << "Caught exception in: " << exc.what();
    }
}

void AlertHandler::handlePortmapAlert(const portmap_alert* alert)
{
    LOG << "Port map alert received. Port: " << alert->external_port;
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
