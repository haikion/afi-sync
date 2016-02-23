#pragma once

#include <QObject>



class  BtsClient : public QObject
{
	Q_OBJECT

	BtsClient& operator=(const BtsClient&) = delete;
	BtsClient(const BtsClient&) = delete;

	public:
	BtsClient(QObject *parent = 0);
	virtual ~BtsClient();

	virtual QString getHost() = 0;
	virtual int getPort() = 0;
	virtual QString getUserName() = 0;
	virtual QString getPassword() = 0;

	virtual bool isClientReady() = 0;

	public slots:
	virtual void exitClient();

	signals:
	void clientStarted();
	void clientExited();
};
