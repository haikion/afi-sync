/*
 * Implements syncronous QNetworkAccessManager.
 * Has its own thread which prevents QNetworkLoop jumping to arbitary locations.
 *
 * TODO: Move to hoxlib
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
    ~SyncNetworkAccessManager() override;

    QNetworkReply* syncGet(QNetworkRequest req, int timeout = DEFAULT_TIMEOUT);
    QByteArray fetchBytes(const QString& url);

private slots:
    void syncGetSlot(const QNetworkRequest& req, QNetworkReply*& reply, int timeout);
    QByteArray fetchBytesSlot(const QString& url);

private:
    static const int DEFAULT_TIMEOUT;

    QThread thread_;
};

#endif // SYNCNETWORKACCESSMANAGER_H
