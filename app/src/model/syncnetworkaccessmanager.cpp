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

QByteArray SyncNetworkAccessManager::fetchBytes(const QString& url)
{
    QByteArray retVal;
    QMetaObject::invokeMethod(this, "fetchBytesSlot", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QByteArray, retVal), Q_ARG(QString, url));
    return retVal;
}

QByteArray SyncNetworkAccessManager::fetchBytesSlot(const QString& url)
{
    QNetworkReply * reply = nullptr;
    syncGetSlot(QNetworkRequest(url), reply, DEFAULT_TIMEOUT);
    if (reply->bytesAvailable() == 0)
    {
        LOG_WARNING << "Failed. url = " << url;
        return QByteArray();
    }
    QByteArray retVal = reply->readAll();
    reply->deleteLater();
    return retVal;
}

void SyncNetworkAccessManager::syncGetSlot(const QNetworkRequest& req, QNetworkReply*& reply, int timeout)
{
    QEventLoop loop;
    reply = get(req);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeout, &loop, &QEventLoop::quit);
    loop.exec();
    if (!reply->isFinished())
    {
        LOG_ERROR << "Request timeout from url " << req.url().url();
    }
}
