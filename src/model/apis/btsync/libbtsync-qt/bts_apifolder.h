#pragma once

#include <QObject>

#include "bts_api.h"

struct BtsApiFolder_private;

class  BtsApiFolder : public QObject
{
	Q_OBJECT

	BtsApiFolder& operator=(const BtsApiFolder&) = delete;
	BtsApiFolder(const BtsApiFolder&) = delete;

	public:
	BtsApiFolder(QObject *parent = 0);
	BtsApiFolder(BtsApi *api, QObject *parent = 0);
	BtsApiFolder(const QString &secret, QObject *parent = 0);
	BtsApiFolder(BtsApi *api, const QString &secret, QObject *parent = 0);
	~BtsApiFolder();

	void setApi(BtsApi *api);
	BtsApi *getApi();

	void setWatchedSecret(const QString &secret);
	QString getWatchedSecret();

	public slots:
	BtsApiNotifier *getFolders();
	BtsApiNotifier *removeFolder();
	BtsApiNotifier *getFiles(const QString &path = QString());
	BtsApiNotifier *setFilePrefs(const QString &path, bool download);
	BtsApiNotifier *getFolderPeers();
	BtsApiNotifier *getFolderPrefs();
	BtsApiNotifier *setFolderPrefs(const QVariantHash &prefs);
	BtsApiNotifier *getFolderHosts();
	BtsApiNotifier *setFolderHosts(const QStringList &hosts);
	BtsApiNotifier *getSecrets(bool encryption = false);

	signals:
	void error(const QString &errorString);
	void getFoldersResult(const QVector<BtsGetFoldersResult> &result, const QString &secret);
	void removeFolderResult(const QString &secret);
	void getFilesResult(const QVector<BtsGetFilesResult> &result, const QString &secret);
	void setFilePrefsResult(const QVector<BtsGetFilesResult> &result, const QString &secret);
	void getFolderPeersResult(const QVector<BtsGetFolderPeersResult> &result, const QString &secret);
	void getFolderPrefsResult(const QVariantHash &prefs, const QString &secret);
	void setFolderPrefsResult(const QVariantHash &prefs, const QString &secret);
	void getFolderHostsResult(const QStringList &hosts, const QString &secret);
	void setFolderHostsResult(const QStringList &hosts, const QString &secret);
	void getSecretsResult(const QString &readWrite, const QString &readOnly, const QString &encryption);

	private:
	BtsApiFolder_private *p;
};
