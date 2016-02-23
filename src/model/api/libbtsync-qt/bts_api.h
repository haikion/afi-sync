#pragma once

#include <QObject>
#include <QVector>
#include <QUuid>

class QJsonDocument;
class QNetworkReply;

class BtsClient;

struct BtsApi_private;

struct  BtsGetFoldersResult
{
	QString dir;
	QString secret;
	qint64 size;
	QString type;
	qint64 files;
	int error;
	int indexing;
};

Q_DECLARE_METATYPE(BtsGetFoldersResult)

struct  BtsGetFolderPeersResult
{
	QString id;
	QString connection;
	QString name;
	qint64 synced;
	qint64 download;
	qint64 upload;
};

Q_DECLARE_METATYPE(BtsGetFolderPeersResult)

enum BtsGetFilesResultType
{
    UnknownBts,
	File,
	EmptyFile,
	Folder
};

struct  BtsGetFilesResult
{
	BtsGetFilesResultType type;
	QString typeString;

	QString name;
	QString state;

	int total_pieces;
	int have_pieces;
	qint64 size;
	bool download;
};

Q_DECLARE_METATYPE(BtsGetFilesResult)

class  BtsApiNotifier : public QObject
{
	Q_OBJECT

	friend class BtsApi;

	BtsApiNotifier& operator=(const BtsApiNotifier&) = delete;
	BtsApiNotifier(const BtsApiNotifier&) = delete;

	BtsApiNotifier(QObject *parent = 0);

	signals:
	void error(const QString &errorString);
	void getFoldersResult(const QVector<BtsGetFoldersResult> &result, const QString &secret);
	void addFolderResult();
	void removeFolderResult(const QString &secret);
	void getFilesResult(const QVector<BtsGetFilesResult> &result, const QString &secret);
	void setFilePrefsResult(const QVector<BtsGetFilesResult> &result, const QString &secret);
	void getFolderPeersResult(const QVector<BtsGetFolderPeersResult> &result, const QString &secret);
	void getFolderPrefsResult(const QVariantHash &prefs, const QString &secret);
	void setFolderPrefsResult(const QVariantHash &prefs, const QString &secret);
	void getFolderHostsResult(const QStringList &hosts, const QString &secret);
	void setFolderHostsResult(const QStringList &hosts, const QString &secret);
	void getSecretsResult(const QString &readWrite, const QString &readOnly, const QString &encryption);
	void getSecretsResultUuid(const QUuid &uuid, const QString &readWrite, const QString &readOnly, const QString &encryption);
	void getPreferencesResult(const QVariantHash &prefs);
	void setPreferencesResult(const QVariantHash &prefs);
	void getOsNameResult(const QString &os);
	void getVersionResult(const QString &version);
	void getSpeedResult(qint64 down, qint64 up);
	void shutdownResult();
};

class  BtsApi : public QObject
{
	Q_OBJECT

	BtsApi& operator=(const BtsApi&) = delete;
	BtsApi(const BtsApi&) = delete;

	void init();

	public:
	BtsApi(QObject *parent = 0);
	BtsApi(BtsClient *client, QObject *parent = 0);
	~BtsApi();

	void setClient(BtsClient *client);
	BtsClient *getClient();

	private:
	bool checkForError(QNetworkReply *reply, BtsApiNotifier *notifier);
	bool checkForError(const QJsonDocument &doc, BtsApiNotifier *notifier);

	public slots:
	BtsApiNotifier *getFolders(const QString &secret = QString());
	BtsApiNotifier *addFolder(const QString &dir, bool selective_sync = false, const QString &secret = QString());
	BtsApiNotifier *addFolder(const QString &dir, const QString &secret);
	BtsApiNotifier *removeFolder(const QString &secret);
	BtsApiNotifier *getFiles(const QString &secret, const QString &path = QString());
	BtsApiNotifier *setFilePrefs(const QString &secret, const QString &path, bool download);
	BtsApiNotifier *getFolderPeers(const QString &secret);
	BtsApiNotifier *getFolderPrefs(const QString &secret);
	BtsApiNotifier *setFolderPrefs(const QString &secret, const QVariantHash &prefs);
	BtsApiNotifier *getFolderHosts(const QString &secret);
	BtsApiNotifier *setFolderHosts(const QString &secret, const QStringList &hosts);
	BtsApiNotifier *getSecrets(bool encryption = false, const QString &secret = QString(), const QUuid &uuid = QUuid());
	BtsApiNotifier *getSecrets(bool encryption, const QUuid &uuid);
	BtsApiNotifier *getSecrets(const QUuid &uuid);
	BtsApiNotifier *getSecrets(const QString &secret);
	BtsApiNotifier *getPreferences();
	BtsApiNotifier *setPreferences(const QVariantHash &prefs);
	BtsApiNotifier *getOsName();
	BtsApiNotifier *getVersion();
	BtsApiNotifier *getSpeed();
	BtsApiNotifier *shutdown();

	signals:
	void error(const QString &errorString);
	void getFoldersResult(const QVector<BtsGetFoldersResult> &result, const QString &secret);
	void addFolderResult();
	void removeFolderResult(const QString &secret);
	void getFilesResult(const QVector<BtsGetFilesResult> &result, const QString &secret);
	void setFilePrefsResult(const QVector<BtsGetFilesResult> &result, const QString &secret);
	void getFolderPeersResult(const QVector<BtsGetFolderPeersResult> &result, const QString &secret);
	void getFolderPrefsResult(const QVariantHash &prefs, const QString &secret);
	void setFolderPrefsResult(const QVariantHash &prefs, const QString &secret);
	void getFolderHostsResult(const QStringList &hosts, const QString &secret);
	void setFolderHostsResult(const QStringList &hosts, const QString &secret);
	void getSecretsResult(const QString &readWrite, const QString &readOnly, const QString &encryption);
	void getSecretsResultUuid(const QUuid &uuid, const QString &readWrite, const QString &readOnly, const QString &encryption);
	void getPreferencesResult(const QVariantHash &prefs);
	void setPreferencesResult(const QVariantHash &prefs);
	void getOsNameResult(const QString &os);
	void getVersionResult(const QString &version);
	void getSpeedResult(qint64 down, qint64 up);
	void shutdownResult();

	private:
	BtsApi_private *p;
};

Q_DECLARE_METATYPE(QVector<BtsGetFoldersResult>)
Q_DECLARE_METATYPE(QVector<BtsGetFilesResult>)
Q_DECLARE_METATYPE(QVector<BtsGetFolderPeersResult>)
