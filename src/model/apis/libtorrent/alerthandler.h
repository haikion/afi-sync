#ifndef ALERTHANDLER_H
#define ALERTHANDLER_H

#include <vector>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <QObject>

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

private:
    void handleAlert(libtorrent::alert* alert);
    void handleFastresumeRejectedAlert(const libtorrent::fastresume_rejected_alert* alert) const;
    void handleListenFailedAlert(const libtorrent::listen_failed_alert* alert) const;
    void handleListenSucceededAlert(const libtorrent::listen_succeeded_alert* alert) const;
    void handleMetadataFailedAlert(const libtorrent::metadata_failed_alert* alert) const;
    void handleMetadataReceivedAlert(const libtorrent::metadata_received_alert* alert) const;
    void handlePortmapAlert(const libtorrent::portmap_alert* alert) const;
    void handlePortmapErrorAlert(const libtorrent::portmap_error_alert* alert) const;
    void handleStorageMoveFailedAlert(const libtorrent::storage_moved_failed_alert* alert);
    void handleStorageMovedAlert(const libtorrent::storage_moved_alert* alert);
    void handleTorrentCheckAlert(const libtorrent::torrent_checked_alert* alert) const;
    void handleTorrentFinishedAlert(const libtorrent::torrent_finished_alert* alert);
};

#endif // ALERTHANDLER_H
