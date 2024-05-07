//Common things in mod and repository

#ifndef ABSTRACTITEM_H
#define ABSTRACTITEM_H

#include <atomic>

#include <QAtomicPointer>
#include <QMutex>
#include <QObject>
#include <QSet>

#include "interfaces/isyncitem.h"

class SyncItem : virtual public ISyncItem
{
public:
    explicit SyncItem(const QString& name);

    QString statusStr() override;
    virtual void processCompletion() = 0;
    void check() override = 0;
    bool optional() override = 0;
    bool ticked() override = 0;
    void checkboxClicked() override = 0;
    [[nodiscard]] QString name() const override;

    void setStatus(const QString& statusStr);
    [[nodiscard]] quint64 fileSize() const;
    void setFileSize(quint64 size);
    [[nodiscard]] QString sizeStr() const override;
    [[nodiscard]] bool active() const override;
    QString progressStr() override = 0;

private:
    QString name_;
    QString status_;
    std::atomic<quint64> fileSize_;
    QMutex statusMutex_;

    static QSet<QString> createActiveStatuses();
};

#endif // ABSTRACTITEM_H
