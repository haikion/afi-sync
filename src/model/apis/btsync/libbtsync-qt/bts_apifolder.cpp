#include <QPointer>

#include "bts_api.h"
#include "bts_global.h"
#include "bts_apifolder.h"


struct BtsApiFolder_private
{
	QPointer<BtsApi> api;
	QString watchedSecret;
};


BtsApiFolder::BtsApiFolder(QObject *parent)
	:QObject(parent)
{
	p = new BtsApiFolder_private();
}

BtsApiFolder::BtsApiFolder(BtsApi *api, QObject *parent)
	:QObject(parent)
{
	p = new BtsApiFolder_private();
	setApi(api);
}

BtsApiFolder::BtsApiFolder(const QString &secret, QObject *parent)
	:QObject(parent)
{
	p = new BtsApiFolder_private();
	p->watchedSecret = secret;
}

BtsApiFolder::BtsApiFolder(BtsApi *api, const QString &secret, QObject *parent)
	:QObject(parent)
{
	p = new BtsApiFolder_private();
	p->watchedSecret = secret;
	setApi(api);
}

BtsApiFolder::~BtsApiFolder()
{
	delete p;
}

void BtsApiFolder::setApi(BtsApi *api)
{
	p->api = api;
}

BtsApi *BtsApiFolder::getApi()
{
	return p->api;
}

void BtsApiFolder::setWatchedSecret(const QString &secret)
{
	p->watchedSecret = secret;
}

QString BtsApiFolder::getWatchedSecret()
{
	return p->watchedSecret;
}

static void assertApi(BtsApiFolder_private *p)
{
	if(p->api.isNull())
		throw BtsException("tried to perform folder action on null BtsApi");

	if(p->watchedSecret.isEmpty())
		throw BtsException("tried to perform folder action without a secret");
}

BtsApiNotifier *BtsApiFolder::getFolders()
{
	assertApi(p);

	BtsApiNotifier *res = p->api->getFolders(p->watchedSecret);

	connect(res, &BtsApiNotifier::getFoldersResult, this, &BtsApiFolder::getFoldersResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::removeFolder()
{
	assertApi(p);

	BtsApiNotifier *res = p->api->removeFolder(p->watchedSecret);

	connect(res, &BtsApiNotifier::removeFolderResult, this, &BtsApiFolder::removeFolderResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::getFiles(const QString &path)
{
	assertApi(p);

	BtsApiNotifier *res = p->api->getFiles(p->watchedSecret, path);

	connect(res, &BtsApiNotifier::getFilesResult, this, &BtsApiFolder::getFilesResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::setFilePrefs(const QString &path, bool download)
{
	assertApi(p);

	BtsApiNotifier *res = p->api->setFilePrefs(p->watchedSecret, path, download);

	connect(res, &BtsApiNotifier::setFilePrefsResult, this, &BtsApiFolder::setFilePrefsResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::getFolderPeers()
{
	assertApi(p);

	BtsApiNotifier *res = p->api->getFolderPeers(p->watchedSecret);

	connect(res, &BtsApiNotifier::getFolderPeersResult, this, &BtsApiFolder::getFolderPeersResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::getFolderPrefs()
{
	assertApi(p);

	BtsApiNotifier *res = p->api->getFolderPrefs(p->watchedSecret);

	connect(res, &BtsApiNotifier::getFolderPrefsResult, this, &BtsApiFolder::getFolderPrefsResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::setFolderPrefs(const QVariantHash &prefs)
{
	assertApi(p);

	BtsApiNotifier *res = p->api->setFolderPrefs(p->watchedSecret, prefs);

	connect(res, &BtsApiNotifier::setFolderPrefsResult, this, &BtsApiFolder::setFolderPrefsResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::getFolderHosts()
{
	assertApi(p);

	BtsApiNotifier *res = p->api->getFolderHosts(p->watchedSecret);

	connect(res, &BtsApiNotifier::getFolderHostsResult, this, &BtsApiFolder::getFolderHostsResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::setFolderHosts(const QStringList &hosts)
{
	assertApi(p);

	BtsApiNotifier *res = p->api->setFolderHosts(p->watchedSecret, hosts);

	connect(res, &BtsApiNotifier::setFolderHostsResult, this, &BtsApiFolder::setFolderHostsResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}

BtsApiNotifier *BtsApiFolder::getSecrets(bool encryption)
{
	assertApi(p);

	BtsApiNotifier *res = p->api->getSecrets(encryption, p->watchedSecret);

	connect(res, &BtsApiNotifier::getSecretsResult, this, &BtsApiFolder::getSecretsResult);
	connect(res, &BtsApiNotifier::error, this, &BtsApiFolder::error);

	return res;
}
