//Common things in mod and repository

#ifndef ABSTRACTITEM_H
#define ABSTRACTITEM_H

#include <atomic>
#include <QObject>
#include <QSet>
#include <QAtomicPointer>
#include <QMutex>
#include "interfaces/isyncitem.h"

class SyncItem : virtual public ISyncItem
{
public:
    explicit SyncItem(const QString& name);

    virtual QString nameText();
    QString statusStr() override;
    virtual void processCompletion() = 0;
    virtual void check() = 0;
    virtual bool optional() = 0;
    virtual bool ticked() = 0;
    virtual void checkboxClicked() = 0;
    QString name() override;

    void setStatus(const QString& statusStr);
    void setName(const QString& name);
    quint64 fileSize() const;
    void setFileSize(const quint64 size);
    QString sizeStr() const override;
    bool active() const override;
    virtual QString progressStr() = 0;

private:
    QString name_;
    QString status_;
    std::atomic<quint64> fileSize_;
    QMutex statusMutex_;
    QMutex nameMutex_;

    static QSet<QString> createActiveStatuses();
};

#endif // ABSTRACTITEM_H
