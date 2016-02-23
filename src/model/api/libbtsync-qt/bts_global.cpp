#include <QProcessEnvironment>
#include <QRegExp>

#include "bts_global.h"


BtsApiKey::BtsApiKey()
{
	key = QString();
}

BtsApiKey::BtsApiKey(const QString &key)
{
	setKey(key);
}

BtsApiKey::BtsApiKey(const char *key)
{
	setKey(key);
}

BtsApiKey &BtsApiKey::operator=(const QString &key)
{
	setKey(key);
	return *this;
}

BtsApiKey &BtsApiKey::operator=(const char *key)
{
	setKey(key);
	return *this;
}

void BtsApiKey::setKey(const QString &newkey)
{
    QRegExp keyRegExp("[a-zA-Z0-9]*", Qt::CaseInsensitive, QRegExp::RegExp2);

	if(!keyRegExp.exactMatch(newkey))
	{
		throw BtsException("Tried to set an invalid BTSync API key.");
	}

	key = newkey;
}

static BtsApiKey globalApiKey;

void BtsGlobal::setApiKey(const BtsApiKey &key)
{
	globalApiKey = key;
}

const BtsApiKey &BtsGlobal::getApiKey()
{
	if(!globalApiKey.isValid())
	{
		throw BtsException("No global BTSync API key has been set yet");
	}

	return globalApiKey;
}


static QString globalBtsExePath;

void BtsGlobal::setBtsyncExecutablePath(const QString &path)
{
	globalBtsExePath = path;
}

QString BtsGlobal::getBtsyncExecutablePath()
{
	if(!globalBtsExePath.isEmpty())
		return globalBtsExePath;

	QProcessEnvironment procEnv = QProcessEnvironment::systemEnvironment();
    if(procEnv.contains("btsYNC_BINARY"))
	{
        globalBtsExePath = procEnv.value("btsYNC_BINARY");
		return globalBtsExePath;
	}

#ifdef BT_DEF_BIN_PATH
	return BT_DEF_BIN_PATH;
#else
	throw BtsException("Unable to find btsync executable path");
#endif
}
