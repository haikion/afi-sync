#ifndef ISYNCITEM_H
#define ISYNCITEM_H

#include <QList>
#include <QString>

namespace SyncStatus {
static const QString CHECKING = QStringLiteral("Checking...");
static const QString CHECKING_PATCHES = QStringLiteral("Checking Patches...");
static const QString DOWNLOADING = QStringLiteral("Downloading...");
static const QString DOWNLOADING_PATCHES = QStringLiteral("Downloading Patch...");
static const QString ERRORED = QStringLiteral("Error: ");
static const QString EXTRACTING_PATCH = QStringLiteral("Extracting Patch...");
static const QString INACTIVE = QStringLiteral("Inactive");
static const QString MOVING_FILES = QStringLiteral("Moving files...");
static const QString NO_FILES = QStringLiteral("No Files");
static const QString NO_PEERS = QStringLiteral("No Peers");
static const QString NO_SYNC_CONNECTION = QStringLiteral("No Sync Connection.");
static const QString PATCHING = QStringLiteral("Patching...");
static const QString PAUSED = QStringLiteral("Paused");
static const QString QUEUED = QStringLiteral("Queued");
static const QString READY = QStringLiteral("Ready");
static const QString READY_PAUSED = QStringLiteral("Ready and Paused");
static const QString STARTING = QStringLiteral("Starting...");
static const QString WAITING = QStringLiteral("Waiting...");
}

/**
 * @brief UI Representation of Sync Item
 */
class ISyncItem
{
public:
    virtual ~ISyncItem() = default;
    virtual QString name() const = 0;
    virtual QString statusStr() = 0;
    virtual QString sizeStr() const = 0;
    virtual bool optional() = 0; // Mutex reading
    virtual bool ticked() = 0;
    virtual void checkboxClicked() = 0;
    virtual void check() = 0;
    // Item is being synchronized
    virtual bool active() const = 0; // TODO: Rename to notReady ?
    virtual QString progressStr() = 0;
};

#endif // ISYNCITEM_H
