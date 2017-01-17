#ifndef ABSTRACTITEM_H
#define ABSTRACTITEM_H

#include <QObject>
#include <QSettings>
#include "treeitem.h"

//Common things in mod and repository

namespace SyncStatus {
    static const QString DOWNLOADING = "Downloading...";
    static const QString READY = "Ready";
    static const QString READY_PAUSED = "Ready and Paused";
    static const QString NO_SYNC_CONNECTION = "No Sync Connection.";
    static const QString CHECKING = "Checking...";
    static const QString WAITING = "Waiting...";
    static const QString PATCHING = "Patching..";
    static const QString NO_PEERS = "No Peers";
    static const QString NO_FILES = "No Files";
    static const QString NOT_IN_SYNC = "Not in Sync";
    static const QString INACTIVE = "Inactive";
    static const QString PAUSED = "Paused";
    static const QString QUEUED = "Queued";
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
    virtual int eta() const;
    virtual void setEta(const int& eta);

    virtual QString status() const;
    void setStatus(const QString& status);
    QString name() const;
    void setName(const QString& name);
    virtual bool ticked() const;
    void setTicked(bool ticked);
    virtual void checkboxClicked();
    unsigned fileSize() const;
    void setFileSize(const unsigned& size);
    QString fileSizeText() const;

protected:
    QSettings* settings() const;

private:
    static QSettings* settings_;
    QString name_;
    QString status_;
    int eta_;
    unsigned fileSize_;
};

#endif // ABSTRACTITEM_H
