#ifndef ISYNCITEM_H
#define ISYNCITEM_H

#include <QList>
#include <QString>

namespace SyncStatus {
    static const QString DOWNLOADING = "Downloading...";
    static const QString DOWNLOADING_PATCHES = "Downloading Patch...";
    static const QString READY = "Ready";
    static const QString READY_PAUSED = "Ready and Paused";
    static const QString NO_SYNC_CONNECTION = "No Sync Connection.";
    static const QString CHECKING = "Checking...";
    static const QString CHECKING_PATCHES = "Checking Patches ...";
    static const QString WAITING = "Waiting...";
    static const QString PATCHING = "Patching..";
    static const QString NO_PEERS = "No Peers";
    static const QString NO_FILES = "No Files";
    static const QString ERRORED = "Error: ";
    static const QString INACTIVE = "Inactive";
    static const QString PAUSED = "Paused";
    static const QString QUEUED = "Queued";
    static const QString STARTING = "Starting...";
}

/**
 * @brief UI Representation of Sync Item
 */
class ISyncItem
{
public:
    virtual QString name() const = 0;
    virtual QString statusStr() = 0;
    virtual QString etaStr() const = 0;
    virtual QString sizeStr() const = 0;
    virtual bool optional() const = 0;
    virtual bool ticked() = 0;
    virtual void checkboxClicked() = 0;
    virtual void check() = 0;
    virtual bool active() const = 0;
    virtual QString progressStr() = 0;
};

#endif // ISYNCITEM_H
