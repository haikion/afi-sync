#include <QEventLoop>
#include <QTimer>
#include "afisynclogger.h"
#include "syncnetworkaccessmanager.h"

const int SyncNetworkAccessManager::DEFAULT_TIMEOUT = 3000;

SyncNetworkAccessManager::SyncNetworkAccessManager(QObject* parent):
    QNetworkAccessManager(parent)
{
    //Required for true blocking
    moveToThread(&thread_);
    thread_.setObjectName("SyncNetworkManager Thread");
    thread_.start();
    LOG << "Thread = " << &thread_ << " id = " << thread_.currentThreadId();
}

SyncNetworkAccessManager::~SyncNetworkAccessManager()
{
    thread_.quit();
    thread_.wait(1000);
    thread_.terminate();
    thread_.wait(1000);
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
        LOG_ERROR << "Request timeout from url " << req.url().url();
}
