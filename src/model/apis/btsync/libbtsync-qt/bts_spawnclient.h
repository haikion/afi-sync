#pragma once

#include <QObject>
#include "bts_client.h"

struct BtsSpawnClient_private;

class  BtsSpawnClient : public BtsClient
{
	Q_OBJECT

	BtsSpawnClient& operator=(const BtsSpawnClient&) = delete;
	BtsSpawnClient(const BtsSpawnClient&) = delete;

	public:
	BtsSpawnClient(QObject *parent = 0);
	~BtsSpawnClient();

	QString getHost();
	int getPort();
	QString getUserName();
	QString getPassword();
	QString getDataPath();

	bool isAutorestart();
	bool isClientReady();
    bool running();

	public slots:
	void exitClient();
	void killClient();
	void exitClient(bool force);
    void startClient(bool force = false);
	void restartClient();

	void setAutorestart(bool autorestart);
	void setPort(int port);
	void setHost(const QString &host);
	void setUsername(const QString &username);
	void setPassword(const QString &password);
	void setDataPath(const QString &dir);

	void randomizePort();
	void randomizeCredentials();
	void randomize();

	private slots:
	void procStarted();
	void procFinished(int exitCode);
	void removeConfig();

	private:
	BtsSpawnClient_private *p;
};
