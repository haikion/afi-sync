#include <QEventLoop>
#include <QTimer>
#include "syncnetworkaccessmanager.h"
#include "debug.h"

const int SyncNetworkAccessManager::DEFAULT_TIMEOUT = 3000;

SyncNetworkAccessManager::SyncNetworkAccessManager(QObject* parent):
    QNetworkAccessManager(parent)
{
    moveToThread(&thread_);
    thread_.start();
}

SyncNetworkAccessManager::~SyncNetworkAccessManager()
{
    thread_.terminate();
}

QNetworkReply* SyncNetworkAccessManager::syncGet(QNetworkRequest req, int timeout)
{
    QNetworkReply* rVal = nullptr;
    QMetaObject::invokeMethod(this, "syncGetSlot", Qt::BlockingQueuedConnection,
                              Q_ARG(QNetworkRequest, req), Q_ARG(QNetworkReply*&, rVal), Q_ARG(int, timeout));
    return rVal;
}

void SyncNetworkAccessManager::syncGetSlot(QNetworkRequest req, QNetworkReply*& reply, int timeout)
{
    QEventLoop loop;
    reply = get(req);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();
    if (!reply->isFinished())
        DBG << "ERROR: Request timeout from url" << req.url().url();
}