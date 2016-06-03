#include <QtDebug>

#include <QTemporaryFile>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QProcess>
#include <QPointer>
#include <QRegExp>
#include <QTimer>
#include <QUuid>
#include <QDir>
#include "bts_spawnclient.h"
#include "bts_global.h"
#include "bts_api.h"
#include "../../../debug.h"
#include "../../../settingsmodel.h"

struct BtsSpawnClient_private
{
	QString host;
	int port;
	QString username;
	QString password;
	QString dataPath;

	int cur_port;
	QString cur_username;
	QString cur_password;

	QTemporaryFile configFile;
	QPointer<QProcess> clientProc;
	QPointer<QTimer> forceKillTimer;

	bool autorestart;
};

BtsSpawnClient::BtsSpawnClient(QObject *parent)
	:BtsClient(parent)
{
	p = new BtsSpawnClient_private();

	p->autorestart = false;
	p->cur_port = 0;
	p->host = "127.0.0.1";
	randomize();

	p->clientProc = nullptr;

	p->forceKillTimer = new QTimer(this);
	p->forceKillTimer->setSingleShot(true);
	p->forceKillTimer->setInterval(2500);
	connect(p->forceKillTimer, SIGNAL(timeout()), this, SLOT(killClient()));
}

BtsSpawnClient::~BtsSpawnClient()
{
	if(p->clientProc)
	{
		BtsApi api(this);

		api.shutdown();
		p->clientProc->waitForFinished(5000);

		p->clientProc->terminate();

		if(!p->clientProc->waitForFinished(5000))
		{
			p->clientProc->kill();
			p->clientProc->waitForFinished(500);
		}

		emit clientExited();
	}

	delete p;
}

void BtsSpawnClient::setAutorestart(bool autorestart)
{
	p->autorestart = autorestart;
}

bool BtsSpawnClient::isAutorestart()
{
	return p->autorestart;
}

QString BtsSpawnClient::getHost()
{
	return "127.0.0.1";
}

int BtsSpawnClient::getPort()
{
	return p->cur_port;
}

QString BtsSpawnClient::getUserName()
{
	return p->cur_username;
}

QString BtsSpawnClient::getPassword()
{
	return p->cur_password;
}

QString BtsSpawnClient::getDataPath()
{
	if(!p->dataPath.isEmpty())
		return p->dataPath;

	QDir dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
	dataPath.mkpath("sync_storage");

	if(!dataPath.cd("sync_storage"))
	{
		qWarning("Failed creating sync storage dir!");
		return QString();
	}

	p->dataPath = dataPath.absolutePath();

	return p->dataPath;
}

bool BtsSpawnClient::isClientReady()
{
	if(!p->clientProc)
		return false;

    return p->clientProc->state() == QProcess::Running;
}

bool BtsSpawnClient::running()
{
    if(!p->clientProc)
        return false;

    return p->clientProc->state() != QProcess::NotRunning;
}

void BtsSpawnClient::exitClient()
{
	exitClient(false);
}

void BtsSpawnClient::killClient()
{
	exitClient(true);
}

void BtsSpawnClient::exitClient(bool force)
{
	if(!p->clientProc)
		return;

	if(force)
	{
		p->clientProc->kill();
		p->forceKillTimer->stop();
	}
	else
	{
		p->clientProc->terminate();
		p->forceKillTimer->start();
	}
}

void BtsSpawnClient::startClient(bool force)
{
    DBG;
    if( (isClientReady() || (p->clientProc && p->clientProc->state() == QProcess::Starting)) && !force)
		return;
    DBG << "Passed";
	QJsonObject configObject;
	configObject.insert("storage_path", getDataPath());
    configObject.insert("download_limit", SettingsModel::maxDownload().toInt());
    configObject.insert("upload_limit", SettingsModel::maxUpload().toInt());
    int port = SettingsModel::port().toInt();
    if (port != 0)
        configObject.insert("listening_port", port);

    configObject.insert("use_gui", true);
    configObject.insert("folder_rescan_interval", 10*60*60);
    configObject.insert("check_for_updates", false);
    configObject.insert("lan_encrypt_data", false);
    configObject.insert("agree_to_EULA", "yes");
    configObject.insert("peer_expiration_days", 1);

    configObject.insert("folder_defaults.delete_to_trash", false);
    configObject.insert("folder_defaults.use_lan_broadcast", false);
    //Warning: config option 'folder_defaults.use_dht' is deprecat
    //configObject.insert("folder_defaults.use_dht", false);

	QJsonObject webuiObject;
	webuiObject.insert("listen", QString("%1:%2").arg(p->host).arg(p->port));
    webuiObject.insert("login", p->username);
    webuiObject.insert("password", p->password);
	webuiObject.insert("api_key", BtsGlobal::getApiKey().toString());
	configObject.insert("webui", webuiObject);

    DBG << "Opening config file";
	p->configFile.open();
	QJsonDocument saveDoc(configObject);
	p->configFile.write(saveDoc.toJson());
	p->configFile.close();

	p->configFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

	p->cur_port = p->port;
	p->cur_password = p->password;
	p->cur_username = p->username;
    if (p->clientProc)
    {
        DBG << "Deleting old process";
        delete p->clientProc;
    }
    DBG << "Creating QProcess";
	p->clientProc = new QProcess(this);
	connect(p->clientProc, SIGNAL(finished(int)), this, SLOT(procFinished(int)));
	connect(p->clientProc, SIGNAL(finished(int)), p->clientProc, SLOT(deleteLater()));
	connect(p->clientProc, SIGNAL(started()), this, SLOT(procStarted()));

#ifdef Q_OS_WIN
# define ARG_PREFIX "/"
#else
# define ARG_PREFIX "--"
#endif

	p->clientProc->setProgram(BtsGlobal::getBtsyncExecutablePath());
	p->clientProc->setArguments(QStringList()
	                            << ARG_PREFIX "nodaemon"
                                << ARG_PREFIX "MINIMIZED"
                                << ARG_PREFIX "config" << p->configFile.fileName()
                                << ARG_PREFIX "log" << "btsync.log");

	p->clientProc->setProcessChannelMode(QProcess::ForwardedChannels);
    DBG << "Starting process";
	p->clientProc->start();
    DBG << "BtSync process started";
}

void BtsSpawnClient::restartClient()
{
	if(!p->clientProc)
	{
		startClient();
	}
	else
	{
		connect(p->clientProc.data(), SIGNAL(finished(int)), this, SLOT(startClient()));
		exitClient();
	}
}

void BtsSpawnClient::setPort(int port)
{
	p->port = port;
}

void BtsSpawnClient::setHost(const QString &host)
{
	p->host = host;
}

void BtsSpawnClient::setUsername(const QString &username)
{
	p->username = username;
}

void BtsSpawnClient::setPassword(const QString &password)
{
	p->password = password;
}

void BtsSpawnClient::setDataPath(const QString &dir)
{
	p->dataPath = dir;
}

void BtsSpawnClient::randomizePort()
{
	p->port = (qrand() % 64000) + 1025;
}

void BtsSpawnClient::randomizeCredentials()
{
	QRegExp removeExp("[\\{\\}\\-]");

	p->username = QUuid::createUuid().toString().remove(removeExp);
	p->password = QUuid::createUuid().toString().remove(removeExp);
}

void BtsSpawnClient::randomize()
{
	randomizePort();
	randomizeCredentials();
}

void BtsSpawnClient::procStarted()
{
	emit clientStarted();
	QTimer::singleShot(10000, this, SLOT(removeConfig()));
}

void BtsSpawnClient::procFinished(int exitCode)
{
	p->forceKillTimer->stop();
	emit clientExited();

    DBG << "btsync finished with code" << exitCode << "autorestart =" << p->autorestart;

    if(exitCode != 0 && p->autorestart)
	{
		QTimer::singleShot(1000, this, SLOT(startClient()));
	}
}

void BtsSpawnClient::removeConfig()
{
	p->configFile.remove();
}
