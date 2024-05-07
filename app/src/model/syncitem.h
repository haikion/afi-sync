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
    void check() override = 0;
    bool optional() override = 0;
    bool ticked() override = 0;
    void checkboxClicked() override = 0;
    QString name() override;

    void setStatus(const QString& statusStr);
    void setName(const QString& name);
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
    QMutex nameMutex_;

    static QSet<QString> createActiveStatuses();
};

#endif // ABSTRACTITEM_H
