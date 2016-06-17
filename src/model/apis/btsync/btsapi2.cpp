#include <limits>
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
#include <QProcess>
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
const int BtsApi2::MIN_INDEXING_TIME = 2000;

BtsApi2::BtsApi2(const QString& username, const QString& password,
                 unsigned port, QObject* parent):
    BtsApi2(createBtsClient(username, password, port), parent)
{
    DBG;
    foldersCache_.second = 0;
}

BtsApi2::BtsApi2(QObject* parent):
    BtsApi2(Constants::DEFAULT_USERNAME, Constants::DEFAULT_PASSWORD, Constants::DEFAULT_PORT, parent)
{
}

BtsApi2::BtsApi2(BtsClient* client, QObject* parent):
    BtsApi(client, parent),
    ready_(false)
{
    DBG << "current thread =" << QThread::currentThread();
    moveToThread(&thread_);
    nam_.moveToThread(&thread_);
    getClient()->moveToThread(&thread_);
    DBG << "Starting thread:" << &thread_;
    thread_.start();
    QMetaObject::invokeMethod(this, "postInit", Qt::QueuedConnection);
}

BtsApi2::~BtsApi2()
{
    DBG;
    QMetaObject::invokeMethod(this, "threadDestructor", connectionType());
    thread_.quit();
    thread_.wait(3000);
}

void BtsApi2::checkFolder(const QString& key)
{
    Q_UNUSED(key);

    //TODO: Implement if needed
    return;
}

void BtsApi2::threadDestructor()
{
    delete heart_;
}

void BtsApi2::postInit()
{
    DBG << "Thread =" << QThread::currentThread();
    heart_ = new Heart(this);
    connect(heart_,  SIGNAL(death()), this, SLOT(start()));
    activateSettings();
    ready_ = true;
    DBG << "emiting initCompleted() token =" << token_;
    emit initCompleted();
}

void BtsApi2::activateSettings()
{
    setDefaultSyncLevel(SyncLevel::DISCONNECTED);
    setShowNotifications(false);
    heart_->reset(30); //Decrease delay after initial setup.
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
    QString dataPath = Constants::SYNC_SETTINGS_PATH;
    DBG << "Creating BtSync data path.";
    QDir().mkpath(dataPath);
    btsclient->setDataPath(dataPath);
    DBG << "Starting BtSync";
    btsclient->startClient();

    return btsclient;
}

bool BtsApi2::folderPaused(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return folder.paused;
}

QString BtsApi2::folderError(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    if (folder.error == 0)
    {
        return "";
    }
    return folder.status;
}

QString BtsApi2::folderPath(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    QString path = QDir(folder.path).absolutePath(); //Cross platform
    return path;
}

void BtsApi2::shutdown()
{
    heart_->stop();
    client()->killClient(); //Exit never succeeds...
    for ( int attempts = 0; client()->running() && attempts < 10; ++attempts)
    {
        DBG << "Waiting for shutdown. attempts =" << attempts;
        QThread::msleep(500);
    }
    //In case BTSync was not spawned by AFISync
    QProcess p;
    p.start("taskkill /F /IM btsync.exe");
    p.waitForFinished(1000);
    token_ = "";
    DBG << "Finished";
}

qint64 BtsApi2::download() const
{
    //TODO: Implement if needed.
    return 0;
}

qint64 BtsApi2::upload() const
{
    //TODO: Implement if needed.
    return 0;
}

void BtsApi2::start()
{
    QMetaObject::invokeMethod(this, "restartSlot", connectionType());
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

bool BtsApi2::folderReady(const QString& key)
{
    if (folderNoPeers(key) || folderChecking(key) || folderEta(key) != 0)
        return false;

    return true;
}

bool BtsApi2::ready()
{
    return ready_;
}

//Chrashes BtSync ...
void BtsApi2::setPort(int port)
{
    QVariantMap map;
    map.insert("listeningport", port);
    QVariantMap rVal = patchVariantMap(map, API_PREFIX + "/client/settings");
    DBG << "rVal =" << rVal;
}


bool BtsApi2::addFolder(const QString& key, const QString& path)
{
    DBG << " path=" << path << " key=" << key;
    if (folderExists(key))
    {
        DBG << "Folder already exists!";
        return false;
    }
    QVariantMap obj;
    obj.insert("force", true);
    obj.insert("secret", key);
    obj.insert("path", path);
    QVariantMap response = postVariantMap(obj, API_PREFIX + "/folders");
    foldersCache_.second = 0; //Force cache refresh
    //Be racist
    setOverwrite(key, true);
    setForce(key, true);
    DBG << "response =" << response << "path =" << path;
    return true;
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
    QVariantMap response = patchVariantMap(obj, API_PREFIX + "/folders/" + fid, 3000);
    DBG << "response =" << response;
    return response;
}


void BtsApi2::setFolderPaused(const QString& key, bool value)
{
    if (!folderExists(key) || folderPaused(key) == value)
    {
        DBG << "Folder does not exist or it is already paused. Returning empty value.";
    }
    QString fid = keyToFid(key);
    QVariantMap obj;
    obj.insert("paused", value);
    QVariantMap response = patchVariantMap(obj, API_PREFIX + "/folders/" + fid, 2000);
    DBG << "response =" << response;
}

bool BtsApi2::removeFolder(const QString& key)
{
    DBG;
    if (!folderExists(key))
    {
        DBG << "Folder does not exist, returning. key =" << key;
        return false;
    }
    DBG << "Folder found, deleting. key =" << key;
    QString fid = keyToFid(key);
    QMetaObject::invokeMethod(this, "httpDeleteSlot", connectionType(),
                              Q_ARG(QString, API_PREFIX + "/folders/" + fid),
                              Q_ARG(unsigned, 10000));
    //Wait for folder to get actually deleted.
    for (int attempts = 0; folderExists(key) && attempts < 20; ++attempts)
    {
        DBG << "Checking if folder removed. attempts =" << attempts;
        fillCache();
    }
    return true;
}

void BtsApi2::httpDeleteSlot(const QString& path, unsigned timeout)
{
    QEventLoop loop;
    QNetworkRequest req = createSecureRequest(path);
    QNetworkReply* reply = nam_.deleteResource(req);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(timeout, &loop, SLOT(quit()));
    loop.exec();
    DBG;
    heart_->beat(reply);
    DBG << "Status code (204=success) =" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    delete reply;
}

void BtsApi2::restartSlot()
{
    shutdown();
    client()->startClient(true);
    heart_->reset();
    activateSettings();
}

QSet<QString> BtsApi2::folderFilesUpper(const QString &key)
{
    QSet<QString> rVal = filesUpperRecursive(key);
    return rVal;
}

QSet<QString> BtsApi2::filesUpperRecursive(const QString& key, const QString& path)
{
    //Sometimes BtSync reports incorrect file listing. This is an attempt to fix it...
    for (int i = 0; i < 10 && ( folderChecking(key) || folderNoPeers(key) || getSyncLevel(key) != SyncLevel::SYNCED); ++i )
    {
        DBG << "Waiting for folder to be ready... i =" << i;
        QThread::sleep(1);
    }
    QSet<QString> rVal;
    QString fid = keyToFid(key);
    QString pathEncoded = path;
    pathEncoded = pathEncoded.replace("\\", "/"); //WTF bts...
    QVariantMap reply  = getVariantMap(API_PREFIX + "/folders/" + fid + "/files?path=" + pathEncoded);
    QVariantMap data = qvariant_cast<QVariantMap>(reply.value("data"));
    QList<QVariant> filesList = qvariant_cast<QList<QVariant>>(data.value("members"));
    DBG << "Checking files in path: " << path << " filesList =" << filesList;
    for (QVariant fileVariant : filesList)
    {
        QVariantMap map = qvariant_cast<QVariantMap>(fileVariant);
        if (map.value("stateid").toInt() == FileState::DELETED)
        {
            continue;
        }
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
            rVal += filesUpperRecursive(key, relpath);
        }
    }
    return rVal;
}

void BtsApi2::fillCache()
{
    foldersCache_.second = QDateTime::currentMSecsSinceEpoch();
    QVariantMap reply;
    for (int attempts = 0; reply.size() == 0 && attempts < 50; ++attempts)
    {
        //In early boot btsync might not be ready yet.
        reply = getVariantMap(API_PREFIX + "/folders/activity");
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
}

FolderHash BtsApi2::getFoldersActivity()
{
    static QMutex mutex;
    if (!mutex.tryLock(5000))
    {
        DBG << "ERROR: locking mutex.";
        return foldersCache_.first;
    }
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

QList<QString> BtsApi2::folderKeys()
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
    heart_->beat(reply);
    if (reply->isRunning())
    {
        DBG << "ERROR: no message from BtSync";
    }
    QByteArray jsonBytes = reply->readAll();
    result = bytesToVariantMap(jsonBytes);
    delete reply;
}

int BtsApi2::folderEta(const QString& key)
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

//Has nothing to do with being completed....
SyncLevel BtsApi2::getSyncLevel(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return static_cast<SyncLevel>(folder.synclevel);
}

bool BtsApi2::folderNoPeers(const QString& key)
{
    BtsFolderActivity folder = getFolderActivity(key);
    return folder.peers.size() == 0;
}

bool BtsApi2::folderChecking(const QString& key)
{
    if (!folderExists(key))
    {
        DBG << "ERROR: Could not find torrent by key:" << key;
        return false;
    }
    BtsFolderActivity folder = getFolderActivity(key);
    if (!folder.indexing)
    {
        indexingStartTime_[key] = std::numeric_limits<int>::max();
        return false; //is not indexing at all.
    }
    //Indexing, let's see if has indexed long enough...
    int deltaTime = runningTimeMs() - indexingStartTime_[key];
    if (deltaTime > MIN_INDEXING_TIME)
    {
        return true; //Has indexed long enough

    }
    indexingStartTime_[key] = runningTimeMs();
    return false; //Has not indexed long enough.
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
    DBG << "Done. result =" << result;
    return result;
}

bool BtsApi2::folderExists(const QString& key)
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

int BtsApi2::getLastModified(const QString& key)
{
    BtsFolderActivity map = getFolderActivity(key);
    return map.last_modfied;
}

void BtsApi2::postVariantMapSlot(const QVariantMap& map, const QString& path, QVariantMap& result)
{
    QJsonDocument jsonDoc = QJsonDocument(QJsonObject::fromVariantMap(map));
    QByteArray jsonBytes = jsonDoc.toJson();
    DBG << "current thread =" << QThread::currentThread()
        << " path =" << path << " jsonBytes =" << jsonBytes;
    QNetworkRequest request = createSecureRequest(path);
    QEventLoop loop;
    QNetworkReply* reply = nam_.post(request, jsonBytes);
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    QTimer::singleShot(TIMEOUT, &loop, SLOT(quit())); //timeout
    loop.exec();
    heart_->beat(reply);
    QByteArray response = reply->readAll();
    DBG << "response =" << response;
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
    DBG;
    heart_->beat(reply);
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

BtsSpawnClient* BtsApi2::client()
{
    BtsSpawnClient* client = static_cast<BtsSpawnClient*>(getClient());
    return client;
}

//Results in:
//Error: 101
//"Don't have permissions to write to the selected folder."
QVariantMap BtsApi2::setOwner(const QString& username)
{
    QVariantMap map;
    //Results in: invalid parameter 'devicename' submitted
    //map.insert("devicename", "Desktop");
    map.insert("username", username);
    QVariantMap rVal = postVariantMap(map, API_PREFIX + "/owner");
    return rVal;
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
    DBG << "Security token succesfully received. token =" << token_;
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
