#include <QtDebug>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QBuffer>
#include <QUrl>
#include <QDir>
#include <QDateTime>
#include <QTimer>
#include "../../debug.h"
#include "../../settingsmodel.h"
#include "libbtsync-qt/bts_global.h"
#include "libbtsync-qt/bts_client.h"
#include "btsapi2.h"
#include "apikey.h"

//Cache update interval in ms
const unsigned BtsApi2::UPDATE_INTERVAL = 1000;
const unsigned BtsApi2::TIMEOUT = 500;
const QString BtsApi2::API_PREFIX = "/api/v2";

BtsApi2::BtsApi2(const QString& username, const QString& password,
                 unsigned port, QObject* parent):
    BtsApi2(createBtsClient(username, password, port), parent)
{
    DBG;
    foldersCache_.second = 0;
}

BtsApi2::BtsApi2(BtsClient* client, QObject* parent):
    BtsApi(client, parent),
    cacheEmpty_(true)
{
    DBG << "current thread =" << QThread::currentThread();
    moveToThread(&thread_);
    nam_.moveToThread(&thread_);
    DBG << "Starting thread:" << &thread_;
    thread_.start();
    QMetaObject::invokeMethod(this, "postInit", Qt::QueuedConnection);
}

BtsApi2::~BtsApi2()
{
    thread_.quit();
    thread_.wait(3000);
}

void BtsApi2::postInit()
{
    DBG << "Thread =" << QThread::currentThread();
    setDefaultSyncLevel(SyncLevel::DISCONNECTED);
    setShowNotifications(false);
    setMaxDownload(SettingsModel::maxDownload().toUInt());
    setMaxUpload(SettingsModel::maxUpload().toUInt());
}

BtsClient* BtsApi2::createBtsClient(const QString& username, const QString& password, unsigned port)
{
    DBG;
    #ifdef Q_OS_WIN
        BtsGlobal::setBtsyncExecutablePath("bin/btsync.exe");
    #elif defined(Q_PROCESSOR_X86_32)
        BtsGlobal::setBtsyncExecutablePath("bin/btsync32");
    #else
        BtsGlobal::setBtsyncExecutablePath("bin/btsync");
    #endif
    BtsGlobal::setApiKey(BTS_API_KEY);
    BtsSpawnClient* btsclient = new BtsSpawnClient();
    btsclient->setUsername(username);
    btsclient->setPassword(password);
    btsclient->setPort(port);
    QString host = "127.0.0.1";
    if (Global::guiless)
    {
        host = "0.0.0.0";
    }
    btsclient->setHost(host);
    btsclient->setAutorestart(false);
    QString dataPath = Constants::BTSYNC_SETTINGS_PATH;
    DBG << "Creating BtSync data path.";
    QDir().mkpath(dataPath);
    btsclient->setDataPath(dataPath);
    DBG << "Starting BtSync";
    btsclient->startClient();

    return btsclient;
}

bool BtsApi2::paused(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return folder.paused;
}

QString BtsApi2::error(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    if (folder.error == 0)
    {
        return "";
    }
    return folder.status;
}

QString BtsApi2::getFolderPath(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    QString path = QDir(folder.path).absolutePath(); //Cross platform
    return path;
}

void BtsApi2::shutdown2()
{
    postVariantMap(QVariantMap(), API_PREFIX + "/client/shutdown");
    token_ = "";
    DBG << "Finished";
}

void BtsApi2::restart2()
{
    BtsSpawnClient* client = static_cast<BtsSpawnClient*>(getClient());
    while ( token_ != "" ) //TODO: Figure out better way to check if btsync is running...
    {
        DBG << "Waiting for shutdown.";
        shutdown2();
        QThread::msleep(500);
    }
    DBG << "Not running. Starting";
    client->startClient(true);
}

void BtsApi2::setMaxUpload(unsigned limit)
{
    QVariantMap obj;
    obj.insert("ulrate", limit);
    patchVariantMap(obj, API_PREFIX + "/client/settings");
}

void BtsApi2::setMaxDownload(unsigned limit)
{
    QVariantMap obj;
    obj.insert("dlrate", limit);
    patchVariantMap(obj, API_PREFIX + "/client/settings");
}


QVariantMap BtsApi2::addFolder(const QString& path, const QString& key, bool force)
{
    DBG << " path=" << path << " key=" << key;
    if (exists(key))
    {
        DBG << "Folder already exists!";
        return QVariantMap();
    }
    QVariantMap obj;
    obj.insert("force", force);
    obj.insert("secret", key);
    obj.insert("path", path);
    QVariantMap response = postVariantMap(obj, API_PREFIX + "/folders");
    foldersCache_.second = 0; //Force cache refresh
    //Be racist
    setOverwrite(key, true);
    setForce(key, true);
    DBG << "response =" << response << "path =" << path;
    return response;
}

QVariantMap BtsApi2::setOverwrite(const QString& key, bool value)
{
    QString fid = keyToFid(key);
    QVariantMap obj;
    obj.insert("overwrite", value);
    QVariantMap response = patchVariantMap(obj, API_PREFIX + "/folders/" + fid);
    DBG << "response =" << response;
    return response;
}

QVariantMap BtsApi2::setForce(const QString& key, bool value)
{
    QString fid = keyToFid(key);
    QVariantMap obj;
    obj.insert("force", value);
    QVariantMap response = patchVariantMap(obj, API_PREFIX + "/folders/" + fid);
    DBG << "response =" << response;
    return response;
}


QVariantMap BtsApi2::setFolderPaused(const QString& key, bool value)
{
    if (!exists(key) || paused(key) == value)
    {
        DBG << "No changes made, returning empty value.";
        return QVariantMap();
    }
    QString fid = keyToFid(key);
    QVariantMap obj;
    obj.insert("paused", value);
    QVariantMap response = patchVariantMap(obj, API_PREFIX + "/folders/" + fid, 10000);
    DBG << "response =" << response;
    return response;
}

void BtsApi2::removeFolder2(const QString& key)
{
    DBG;
    if (!exists(key))
    {
        DBG << "Folder does not exist, returning. key =" << key;
        return;
    }
    DBG << "Folder found, deleting. key=" << key;
    QString fid = keyToFid(key);
    QMetaObject::invokeMethod(this, "httpDeleteSlot", connectionType(),
                              Q_ARG(QString, API_PREFIX + "/folders/" + fid),
                              Q_ARG(unsigned, 10000));
    //Wait for folder to get actually deleted.
    int attempts = 0;
    while (exists(key) && attempts < 20)
    {
        DBG << "Checking if folder removed. attempts =" << attempts;
        fillCache();
        ++attempts;
    }
}

void BtsApi2::httpDeleteSlot(const QString& path, unsigned timeout)
{
    QEventLoop loop;
    QNetworkRequest req = createSecureRequest(path);
    QNetworkReply* reply = nam_.deleteResource(req);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();
    DBG << "Status code (204=success) =" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    delete reply;
}

QSet<QString> BtsApi2::getFilesUpper(const QString& key, const QString& path)
{
    QSet<QString> rVal;
    QString fid = keyToFid(key);
    QString pathEncoded = path;
    pathEncoded = pathEncoded.replace("\\", "/"); //WTF bts...
    QVariantMap reply  = getVariantMap(API_PREFIX + "/folders/" + fid + "/files?path=" + pathEncoded);
    QVariantMap data = qvariant_cast<QVariantMap>(reply.value("data"));
    QList<QVariant> filesList = qvariant_cast<QList<QVariant>>(data.value("members"));
    for (QVariant fileVariant : filesList)
    {
        QVariantMap map = qvariant_cast<QVariantMap>(fileVariant);
        QString btsPath = map.value("path").toString();
        QDir dir(btsPath.replace(0,4,""));
        QString dirStr = dir.absolutePath();
        if (!QFileInfo(dirStr).isDir())
        {
            rVal.insert(dirStr.toUpper());
        }
        else
        {
            QString relpath = map.value("relpath").toString();
            rVal += getFilesUpper(key, relpath);
        }
    }
    return rVal;
}

void BtsApi2::fillCache()
{
    foldersCache_.second = QDateTime::currentMSecsSinceEpoch();
    QVariantMap reply;
    int attempts = 0;
    while (reply.size() == 0 && attempts < 50)
    {
        //In early boot btsync might not be ready yet.
        reply = getVariantMap(API_PREFIX + "/folders/activity");
        ++attempts;
    }
    QVariantMap data = qvariant_cast<QVariantMap>(reply.value("data"));
    QList<QVariant> variants = qvariant_cast<QList<QVariant>>(data.value("folders"));

    FolderHash newHash;
    for (QVariant variant : variants)
    {
        //TODO: Record everything
        QVariantMap folderMap = qvariant_cast<QVariantMap>(variant);
        BtsFolderActivity folder;
        folder.access = qvariant_cast<int>(folderMap.value("access"));
        folder.accessname = qvariant_cast<QString>(folderMap.value("accessname"));
        folder.archive = qvariant_cast<QString>(folderMap.value("archive"));
        folder.archive_files = qvariant_cast<int>(folderMap.value("archive_files"));
        folder.canencrypt = qvariant_cast<bool>(folderMap.value("canencrypt"));
        folder.data_added = qvariant_cast<int>(folderMap.value("data_added"));
        folder.disabled = qvariant_cast<bool>(folderMap.value("disabled"));
        folder.down_eta = qvariant_cast<int>(folderMap.value("down_eta"));
        folder.down_speed = qvariant_cast<int>(folderMap.value("down_speed"));
        folder.down_status = qvariant_cast<int>(folderMap.value("down_status"));
        folder.encryptedsecret = qvariant_cast<QString>(folderMap.value("encryptedsecret"));
        folder.error = qvariant_cast<int>(folderMap.value("error"));
        folder.files = qvariant_cast<int>(folderMap.value("files"));
        folder.hash = qvariant_cast<QString>(folderMap.value("hash"));
        folder.has_key = qvariant_cast<bool>(folderMap.value("has_key"));
        folder.id = qvariant_cast<QString>(folderMap.value("id"));
        folder.indexing = qvariant_cast<bool>(folderMap.value("indexing"));
        folder.ismanaged = qvariant_cast<bool>(folderMap.value("ismanaged"));
        //FIXME: last_modified receives nothing
        folder.last_modfied = qvariant_cast<int>(folderMap.value("last_modified", -404));
        folder.name = qvariant_cast<QString>(folderMap.value("name"));
        folder.path = qvariant_cast<QString>(folderMap.value("path"));
        folder.peers = qvariant_cast<QList<QVariant>>(folderMap.value("peers"));
        folder.down_speed = qvariant_cast<int>(folderMap.value("down_speed"));
        folder.readonlysecret = qvariant_cast<QString>(folderMap.value("readonlysecret"));
        folder.status = qvariant_cast<QString>(folderMap.value("status"));
        folder.secret = qvariant_cast<QString>(folderMap.value("secret"));
        folder.synclevel = qvariant_cast<int>(folderMap.value("synclevel"));
        folder.paused = qvariant_cast<bool>(folderMap.value("paused", false));

        newHash.insert(folder.readonlysecret, folder);
    }
    foldersCache_.first = newHash;
    cacheEmpty_ = false;
    emit cacheFilled();
}

FolderHash BtsApi2::getFoldersActivity()
{
    static QMutex mutex;
    mutex.lock();
    unsigned timePassed = QDateTime::currentMSecsSinceEpoch() - foldersCache_.second;
    if (timePassed < UPDATE_INTERVAL)
    {
        //DBG << "using cached data";
        mutex.unlock();
        return foldersCache_.first;
    }
    fillCache();
    mutex.unlock();
    return foldersCache_.first;
}

QList<QString> BtsApi2::getFolderKeys()
{
    QList<QString> rVal;
    for (BtsFolderActivity folder : getFoldersActivity())
    {
        rVal.append(folder.readonlysecret);
    }
    return rVal;
}

void BtsApi2::getVariantMapSlot(const QString& path, unsigned timeout, QVariantMap& result)
{
    QNetworkAccessManager* nam = &nam_;
    QNetworkRequest req = createUnauthenticatedRequest(path);
    QEventLoop loop;

    QNetworkReply* reply = nam->get(req);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit())); //Timeout
    loop.exec(); //Will dead lock if main thread dead locks
    QByteArray jsonBytes = reply->readAll();
    result = bytesToVariantMap(jsonBytes);
    delete reply;
}

int BtsApi2::getFolderEta(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return folder.down_eta;
}

BtsFolderActivity BtsApi2::getFolderActivity(const QString& key)
{
    FolderHash folders = getFoldersActivity();
    FolderHash::const_iterator folderIterator =  folders.find(key);
    if (folderIterator == folders.end())
    {
        return BtsFolderActivity();
    }
    return folderIterator.value();
}

SyncLevel BtsApi2::getSyncLevel(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return static_cast<SyncLevel>(folder.synclevel);
}

bool BtsApi2::noPeers(const QString &key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return folder.peers.size() == 0;
}

bool BtsApi2::isIndexing(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return folder.indexing;
}


QString BtsApi2::keyToFid(const QString& key)
{
    BtsFolderActivity folders = getFolderActivity(key);
    return folders.id;
}

QVariantMap BtsApi2::patchVariantMap(const QVariantMap& map, const QString& path, unsigned timeout)
{
    QVariantMap result;

    DBG << "invoke, current thread =" << QThread::currentThread();
    QMetaObject::invokeMethod(this, "patchVariantMapSlot", connectionType(),
                              Q_ARG(const QVariantMap, map),
                              Q_ARG(const QString, path), Q_ARG(unsigned, timeout),
                              Q_RETURN_ARG(QVariantMap&, result));
    DBG << "Done";
    return result;
}

bool BtsApi2::exists(const QString& key)
{
    FolderHash folders = getFoldersActivity();
    if (folders.contains(key))
    {
        return true;
    }
    return false;
}

QVariantMap BtsApi2::setShowNotifications(bool value)
{
    QVariantMap map;
    map.insert("show_notifications", value);
    return patchVariantMap(map, API_PREFIX + "/client/settings");
}

QVariantMap BtsApi2::setDefaultSyncLevel(SyncLevel level)
{
    QVariantMap map;
    map.insert("synclevel", level);
    QVariantMap response = patchVariantMap(map, API_PREFIX + "/client/defaultsynclevel", 5000);
    DBG << "response =" << response;
    return response;
}

int BtsApi2::getLastModified(const QString &key)
{
    BtsFolderActivity map = getFolderActivity(key);
    return map.last_modfied;
}

void BtsApi2::postVariantMapSlot(const QVariantMap& map, const QString& path, QVariantMap& result)
{
    DBG << "current thread =" << QThread::currentThread();
    QJsonDocument jsonDoc = QJsonDocument(QJsonObject::fromVariantMap(map));
    QByteArray jsonBytes = jsonDoc.toJson();
    QNetworkRequest request = createSecureRequest(path);
    QEventLoop loop;
    QNetworkReply* reply = nam_.post(request, jsonBytes);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(TIMEOUT, &loop, SLOT(quit())); //timeout
    loop.exec();
    QByteArray response = reply->readAll();
    result = bytesToVariantMap(response);
}

void BtsApi2::patchVariantMapSlot(const QVariantMap& map, const QString& path,
                                     unsigned timeout, QVariantMap& result)
{
    DBG;
    QJsonDocument jsonDoc = QJsonDocument(QJsonObject::fromVariantMap(map));
    QByteArray jsonBytes = jsonDoc.toJson();
    DBG << "jsonDoc=" << jsonBytes;
    QNetworkRequest request = createSecureRequest(path);
    QBuffer* buffer = new QBuffer();
    //Hack to overcome lack of HTTP PATCH in QNetworkAccessManager
    buffer->open((QBuffer::ReadWrite));
    buffer->write(jsonBytes);
    buffer->seek(0);
    QEventLoop loop;
    QNetworkReply* reply = nam_.sendCustomRequest(request, "PATCH", buffer);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();
    QByteArray response = reply->readAll();
    result = bytesToVariantMap(response);
    DBG << "result =" << result;
}

QVariantMap BtsApi2::bytesToVariantMap(const QByteArray& jsonBytes) const
{
    QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    QVariantMap rVal = qvariant_cast<QVariantMap>(doc.toVariant());
    return rVal;
}

Qt::ConnectionType BtsApi2::connectionType()
{
    Qt::ConnectionType type = Qt::BlockingQueuedConnection;
    if (QThread::currentThread() == &thread_)
        type = Qt::DirectConnection;

    return type;
}

QVariantMap BtsApi2::postVariantMap(const QVariantMap& map, const QString& path)
{
    QVariantMap result;
    DBG << "invoke, current thread =" << QThread::currentThread();

    QMetaObject::invokeMethod(this, "postVariantMapSlot", connectionType(),
                              Q_ARG(const QVariantMap, map), Q_ARG(const QString, path),
                              Q_RETURN_ARG(QVariantMap&, result));
    return result;
}

QVariantMap BtsApi2::getVariantMap(const QString& path, unsigned timeout)
{
    QVariantMap result;

    QMetaObject::invokeMethod(this, "getVariantMapSlot", connectionType(),
                              Q_ARG(QString, path), Q_ARG(unsigned, timeout),
                              Q_RETURN_ARG(QVariantMap&, result));
    return result;
}

QNetworkRequest BtsApi2::createSecureRequest(QString path)
{
    unsigned a = 0;
    while (token_ == "")
    {
        token_ = token();
        ++a;
        if (a > 20)
        {
            DBG << "Error: Failure fetching security token.";
            return QNetworkRequest();
        }
    }
    path = path + "?token=" + token_;
    QNetworkRequest request = createUnauthenticatedRequest(path);
    return request;
}

QNetworkRequest BtsApi2::createUnauthenticatedRequest(const QString& url)
{
    BtsClient* client = getClient();

    QUrl urlObj;
    urlObj.setUrl(url);
    urlObj.setScheme("http");
    urlObj.setHost(client->getHost());
    urlObj.setPort(client->getPort());
    urlObj.setUserName(client->getUserName());
    urlObj.setPassword(client->getPassword());
    QNetworkRequest request(urlObj);
    request.setRawHeader("Content-Type", "application/json");
    //DBG << "url = " << urlObj.toDisplayString();
    return request;
}

QString BtsApi2::token()
{
    QVariantMap variantMap = getVariantMap(API_PREFIX + "/token", 10000);
    DBG << "variantMap =" << variantMap;
    QVariantMap data = qvariant_cast<QVariantMap>(variantMap.value("data"));
    QString token = qvariant_cast<QString>(data.value("token"));
    return token;
}
