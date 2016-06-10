/*
 * Implements syncronous QNetworkAccessManager.
 * Has its own thread which prevents QNetworkLoop jumping to arbitary locations.
 */

#ifndef SYNCNETWORKACCESSMANAGER_H
#define SYNCNETWORKACCESSMANAGER_H

#include <QObject>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class SyncNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    explicit SyncNetworkAccessManager(QObject* parent = nullptr);
    ~SyncNetworkAccessManager();

    QNetworkReply* syncGet(QNetworkRequest req, int timeout = DEFAULT_TIMEOUT);

private slots:
    void syncGetSlot(QNetworkRequest req, QNetworkReply*& reply, int timeout);

private:
    static const int DEFAULT_TIMEOUT;

    QThread thread_;
};

#endif // SYNCNETWORKACCESSMANAGER_H
