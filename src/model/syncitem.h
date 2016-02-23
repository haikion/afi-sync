#ifndef ABSTRACTITEM_H
#define ABSTRACTITEM_H

#include <QObject>
#include <QSettings>
#include "treeitem.h"

//Common things in mod and repository

namespace SyncStatus {
    static const QString DOWNLOADING = "Downloading...";
    static const QString READY = "Ready";
    static const QString NO_BTSYNC_CONNECTION = "Error: No BTSync connection.";
    static const QString INDEXING = "Indexing...";
    static const QString WAITING = "Waiting...";
    static const QString NO_PEERS = "No peers";
    static const QString NO_FILES = "No Files";
    static const QString NOT_IN_BTSYNC = "Not in BTSync";
    static const QString INACTIVE = "Inactive";
    static const QString PAUSED = "paused";
}

class SyncItem : public TreeItem, public QObject
{
public:
    explicit SyncItem(const QString& name, TreeItem* parentItem = nullptr);

    virtual QString checkText();
    virtual QString nameText();
    virtual QString progressText();
    virtual QString statusText();
    virtual QString startText() = 0;
    virtual QString joinText() = 0;
    virtual void setEta(const int& eta);

    QString status() const;
    void setStatus(const QString& status);
    int eta() const;
    QString name() const;
    void setName(const QString& name);
    virtual bool checked() const;
    void setChecked(bool checked);
    virtual void checkboxClicked();

private:
    static QSettings* settings_;
    QString name_;
    QString status_;
    BtsApi2* btsync_;
    int eta_;
    //QSettings settings_; <- Causes slow construction.
};

#endif // ABSTRACTITEM_H
