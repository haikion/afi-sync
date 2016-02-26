#pragma once

#include <QString>
#include <exception>

class  BtsException : public std::exception
{
	public:
	BtsException(const QString &msg) noexcept
		:msg(msg),constMsg(msg.toUtf8())
	{}

	~BtsException() noexcept
	{}

	QString getMessage() const noexcept
	{
		return msg;
	}

	const char *what() const noexcept
	{
		return constMsg.constData();
	}

	private:
	QString msg;
	QByteArray constMsg;
};

class  BtsApiKey
{
	public:
	BtsApiKey();
	BtsApiKey(const QString &key);
	BtsApiKey(const char *key);

	BtsApiKey& operator=(const QString &key);
	BtsApiKey& operator=(const char *key);

	operator QString() const { return key; }
	QString toString() const { return key; }

	inline bool isValid() { return !key.isEmpty(); }

	private:
	void setKey(const QString &newkey);

	private:
	QString key;
};

class  BtsGlobal
{
	public:
	static void setApiKey(const BtsApiKey &key);
	static const BtsApiKey &getApiKey();

	static void setBtsyncExecutablePath(const QString &path);
	static QString getBtsyncExecutablePath();
};
