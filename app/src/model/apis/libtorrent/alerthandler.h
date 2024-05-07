#ifndef ALERTHANDLER_H
#define ALERTHANDLER_H

#include <vector>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/performance_counters.hpp>
#include <QObject>
#include "speedcalculator.h"

class AlertHandler : public QObject
{
    Q_OBJECT

public:
    explicit AlertHandler(QObject* parent = nullptr);

public:
    void handleAlerts(const std::vector<libtorrent::alert*>* alerts);

signals:
    void storageMoved(libtorrent::torrent_handle handle);
    void storageMoveFailed(libtorrent::torrent_handle handle);
    void uploadAndDownloadChanged(int64_t ul, int64_t dl);

private:
    int uploadIdx_;
    int downloadIdx_;
    SpeedCalculator speedCalculator_;

    void handleAlert(libtorrent::alert* alert);
    static void handleFastresumeRejectedAlert(const libtorrent::fastresume_rejected_alert* alert);
    static void handleListenFailedAlert(const libtorrent::listen_failed_alert* alert);
    static void handleListenSucceededAlert(const libtorrent::listen_succeeded_alert* alert);
    static void handleMetadataFailedAlert(const libtorrent::metadata_failed_alert* alert);
    static void handleMetadataReceivedAlert(const libtorrent::metadata_received_alert* alert);
    void handlePortmapAlert(const libtorrent::portmap_alert* alert);
    static void handlePortmapErrorAlert(const libtorrent::portmap_error_alert* alert);
    void handleSessionStatsAlert(const libtorrent::session_stats_alert* alert);
    void handleStorageMoveFailedAlert(const libtorrent::storage_moved_failed_alert* alert);
    void handleStorageMovedAlert(const libtorrent::storage_moved_alert* alert);
    static void handleTorrentCheckAlert(const libtorrent::torrent_checked_alert* alert);
    static void handleTorrentFinishedAlert(const libtorrent::torrent_finished_alert* alert);
};

#endif // ALERTHANDLER_H
