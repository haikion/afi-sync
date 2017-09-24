#ifndef BISSIGNATURECHECKER_H
#define BISSIGNATURECHECKER_H

#include <QString>
#include <QSet>
#include <QObject>
#include <QQueue>
#include <QSharedPointer>
#include "sigcheckprocess.h"

static void doDeleteLater(QObject* obj)
{
    obj->deleteLater();
}

class BisSignatureChecker : public QObject
{
    Q_OBJECT

public:
    BisSignatureChecker(QObject* parent = nullptr);
    QSharedPointer<SigCheckProcess> check(const QString& modPath, const QSet<QString>& filePaths);
    bool isQueued(const QString& modPath) const;
    bool isChecking(const QString& modPath) const;

signals:
    void checkDone();

private:
    QQueue<QSharedPointer<SigCheckProcess>> executionQueue_;

    QSharedPointer<SigCheckProcess> createProcess(const QString& modPath, const QSet<QString>& filePaths) const;

private slots:
    void processQueue();
};

#endif // BISSIGNATURECHECKER_H
