// Implements some Api V2 features
#ifndef BTSAPI2_H
#define BTSAPI2_H

#include <QPair>
#include <QObject>
#include <QList>
#include <QVariantMap>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QSet>
#include <QTimer>
#include <QThread>
#include "../heart.h"
#include "libbtsync-qt/bts_api.h"
#include "btsfolderactivity.h"

//Folder activity holder for optimized search by readonlysecret a.k.a key
typedef QHash<QString, BtsFolderActivity> FolderHash;
//Data and timestamp (unix time).
typedef QPair<FolderHash,unsigned> FoldersActivityCache;

enum SyncLevel {
    DISCONNECTED = 0,
    CONNECTED = 1,
    SYNCED = 2
};

class BtsApi2: public BtsApi
{
    Q_OBJECT

public:
    BtsApi2(const QString& username, const QString& password, unsigned port, QObject* parent = nullptr);
    BtsApi2(BtsClient* client, QObject* parent = nullptr);
    ~BtsApi2();

    QList<BtsFolderActivity>  getFoldersActivityResult();
    FolderHash getFoldersActivity(); //Caching
    QList<QString> getFolderKeys();
    BtsFolderActivity getFolderActivity(const QString& key);
    SyncLevel getSyncLevel(const QString& key);
    bool noPeers(const QString& key);
    bool isIndexing(const QString& key);
    QVariantMap setFolderPaused(const QString& key, bool value);
    int getFolderEta(const QString& key);
    void removeFolder2(const QString& key);
    QSet<QString> getFilesUpper(const QString& key, const QString& path = "");
    QVariantMap setOverwrite(const QString &key, bool value);
    QVariantMap setForce(const QString &key, bool value);
    bool exists(const QString& key);
    QVariantMap setShowNotifications(bool value);
    int getLastModified(const QString& key);
    bool paused(const QString& key);
    QString error(const QString& key);
    QString getFolderPath(const QString& key);
    void shutdown2();
    void setMaxUpload(unsigned limit);
    void setMaxDownload(unsigned limit);

public slots:
    QVariantMap addFolder(const QString &path, const QString& key, bool force = false);
    QString token();
    QVariantMap setDefaultSyncLevel(SyncLevel level);
    void restart2();

signals:
    void cacheFilled();

private slots:
    void postInit();
    void fillCache();
    //These slots are called in thread_
    void getVariantMapSlot(const QString& path, unsigned timeout,
                                  QVariantMap& result);
    void postVariantMapSlot(const QVariantMap& map, const QString& path, QVariantMap& result);
    void patchVariantMapSlot(const QVariantMap& map, const QString& path,
                         unsigned timeout, QVariantMap& result);
    void httpDeleteSlot(const QString& path, unsigned timeout = TIMEOUT);

private:
    static const unsigned UPDATE_INTERVAL; //Cache update interval in seconds
    static const QString API_PREFIX;
    static const unsigned TIMEOUT;

    bool cacheEmpty_;
    QString token_;
    QNetworkAccessManager nam_;
    FoldersActivityCache foldersCache_;
    QThread thread_;
    Heart* heart_;

    QNetworkRequest createUnauthenticatedRequest(const QString& url);
    QNetworkRequest createSecureRequest(QString path);
    QVariantMap bytesToVariantMap(const QByteArray& jsonBytes) const;
    QVariantMap postVariantMap(const QVariantMap& map, const QString& path);
    QVariantMap getVariantMap(const QString& path, unsigned timeout = TIMEOUT);
    QString keyToFid(const QString& key);
    QVariantMap patchVariantMap(const QVariantMap& map, const QString& path, unsigned timeout = TIMEOUT);
    BtsClient*createBtsClient(const QString& username, const QString& password, unsigned port);
    Qt::ConnectionType connectionType();
};

#endif // BTSAPI2_H
