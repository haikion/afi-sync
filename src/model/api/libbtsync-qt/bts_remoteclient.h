#pragma once

#include <QObject>
#include "bts_client.h"

struct BtsRemoteClient_private;

class  BtsRemoteClient : public BtsClient
{
	Q_OBJECT

	BtsRemoteClient& operator=(const BtsRemoteClient&) = delete;
	BtsRemoteClient(const BtsRemoteClient&) = delete;

	void notify();

	public:
	BtsRemoteClient(QObject *parent = 0);
	BtsRemoteClient(QString host, int port, QObject *parent = 0);
	BtsRemoteClient(QString host, int port, QString username, QString password, QObject *parent = 0);
	~BtsRemoteClient();

	QString getHost();
	void setHost(const QString &host);

	int getPort();
	void setPort(int port);

	QString getUserName();
	void setUserName(const QString &username);

	QString getPassword();
	void setPassword(const QString &password);

	bool isClientReady();

	private slots:
	void notifOnce();

	private:
	BtsRemoteClient_private *p;
};
