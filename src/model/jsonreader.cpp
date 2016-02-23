//TODO: Serialize into QByteArray for faster reload
#include <QFile>
#include <QFileInfo> //Debug prints
#include <QTime> //Performance testing
#include <QNetworkReply>
#include <QVariantMap>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include "mod.h"
#include "repository.h"
#include "jsonreader.h"
#include "debug.h"

const QString JsonReader::FILE_PATH = "settings/repositories.json";
const QString JsonReader::DOWNLOADED_PATH = FILE_PATH + "_new";

//Fastest
void JsonReader::fillEverything(RootItem* root)
{
    QTime time;
    time.start();
    QJsonDocument docElement = openJsonFile(FILE_PATH);
    QVariantMap jsonMap = qvariant_cast<QVariantMap>(docElement.toVariant());
    QString updateUrl = qvariant_cast<QString>(jsonMap.value("updateUrl"));
    if ( updateJson(updateUrl) )
    {
        //Read updated (shutdowns if invalid json file)
        docElement = openJsonFile(DOWNLOADED_PATH);
        QFile::remove(FILE_PATH);
        //Valid json, replace old one.
        QFile::rename(DOWNLOADED_PATH, FILE_PATH);
        jsonMap = qvariant_cast<QVariantMap>(docElement.toVariant());
    }
    QList<QVariant> repositories = qvariant_cast<QList<QVariant>>(jsonMap.value("repositories"));
    DBG << "repositories size =" << repositories.size() << " updateurl=" << updateUrl;
    QHash<QString, Mod*> modHash;
    for (const QVariant& repoVar : repositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        DBG << "qvariant_cast<QVariantMap>(repoVar);";
        QString name = qvariant_cast<QString>(repository.value("name"));
        QString serverAddress = qvariant_cast<QString>(repository.value("serverAddress"));
        unsigned serverPort = qvariant_cast<unsigned>(repository.value("serverPort"));
        QString password = qvariant_cast<QString>(repository.value("password", ""));
        DBG << "Repo Json parsed";
        Repository* newRepo = new Repository(name, serverAddress, serverPort, password, root);
        DBG << "repo created";
        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
        for (QVariant modVar : mods)
        {
            QVariantMap mod = qvariant_cast<QVariantMap>(modVar);

            QString key = qvariant_cast<QString>(mod.value("key"));
            DBG << "key parsed. key=" << key;
            QHash<QString, Mod*>::const_iterator it = modHash.find(key);
            Mod* newMod;
            if (it != modHash.end())
            {
                DBG << "Already added";
                //Use existing one from another repo.
                newMod = it.value();
            }
            else
            {
                DBG << "Creating new mod...";
                QString modName = qvariant_cast<QString>(mod.value("name"));
                bool isOptional = qvariant_cast<bool>(mod.value("optional", false));
                DBG << "Parsed mod parameters";
                newMod = new Mod(modName, key, isOptional);
                DBG << "New mod object created.";
                modHash.insert(key, newMod);
                DBG << "added to modhash";
            }
            DBG << "appending mod name =" << newMod->name() << " key =" << newMod->key();
            newRepo->appendMod(newMod);
        }

        DBG << "appending repo name =" << name << " address =" << serverAddress
            << " port =" << QString::number(serverPort);
        root->appendChild(newRepo);
    }
}


QJsonDocument JsonReader::openJsonFile(const QString& path)
{
    //QString absolutePath = QCoreApplication::applicationFilePath() + "/" +path;
    QFile file(path);
    DBG << "opening file. path =" << QFileInfo(path).absoluteFilePath();
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        DBG << "ERROR: failed to open json file: " << path << " file.exists =" << file.exists();
        exit(2);
        return QJsonDocument();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
    {
        DBG << "ERROR: error parsing json file: " << path;
        exit(3);
        return QJsonDocument();
    }
    DBG << "Doc: " << doc.toJson();
    file.close();

    return doc;
}

//Downloads new json file.
bool JsonReader::updateJson(const QString& url)
{
    QNetworkAccessManager nam;
    QEventLoop loop;
    QNetworkReply* reply = nam.get(QNetworkRequest(url));
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec(); //Wait for response.
    QFile file(DOWNLOADED_PATH);
    if (reply->bytesAvailable() == 0 || !file.open(QIODevice::WriteOnly))
    {
        DBG << " failed";
        return false;
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();
    return true;
}
/*
//Unused alternatives.
void JsonReader::fillRepositories(RootItem* root)
{
    QTime time;
    time.start();
    QJsonDocument docElement = openJsonFile(FILE_PATH);
    QVariantMap jsonMap = qvariant_cast<QVariantMap>(docElement.toVariant());
    QString updateUrl = qvariant_cast<QString>(jsonMap.value("updateUrl"));
    QList<QVariant> repositories = qvariant_cast<QList<QVariant>>(jsonMap.value("repositories"));
    for (const QVariant& repoVar : repositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        DBG << "qvariant_cast<QVariantMap>(repoVar);";
        QString name = qvariant_cast<QString>(repository.value("name"));
        repositoryHash_.insert(name, repository);
        QString serverAddress = qvariant_cast<QString>(repository.value("serverAddress"));
        unsigned serverPort = qvariant_cast<unsigned>(repository.value("serverPort"));
        QString password = qvariant_cast<QString>(repository.value("password", ""));
        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
        DBG << "Repo Json parsed";
        Repository* newRepo = new Repository(name, serverAddress, serverPort, password, root);
        DBG << "repo created";
        root->appendChild(newRepo);
    }
}

void JsonReader::fillMods(Repository* repo)
{
    QTime time;
    time.start();
    QString name = repo->name();
    QHash<QString, QVariantMap>::const_iterator repoIt = repositoryHash_.find(name);
    if (repoIt == repositoryHash_.end())
    {
        DBG << "No such repo! name=" << name;
        return;
    }
    QVariantMap repository = repoIt.value();
    QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
    for (QVariant modVar : mods)
    {
        QVariantMap mod = qvariant_cast<QVariantMap>(modVar);

        QString key = qvariant_cast<QString>(mod.value("key"));
        QHash<QString, Mod*>::const_iterator it = modHash_.find(key);
        Mod* newMod;

        if (it == modHash_.end())
        {
            QString modName = qvariant_cast<QString>(mod.value("name"));
            bool isOptional = qvariant_cast<bool>(mod.value("optional", false));
            newMod = new Mod(modName, key, isOptional);
            repo->appendChild(newMod);
            modHash_.insert(key, newMod);
        }
        else
        {
            //Use existing one from another repo.
            newMod = it.value();
        }
        DBG << "appending mod name =" << newMod->name() << " key =" << newMod->key();
        repo->appendChild(newMod); //FIXME: appendChild(newMod) calls TreeItem::appendChild (wtf...)
    }
}


//Attempt to read json with just QJson objects
//Result: Slower than fillEverything()
void JsonReader::fillEverything2(RootItem* root)
{
    QTime time;
    time.start();
    QJsonDocument docElement = openJsonFile(FILE_PATH);
    QJsonObject jsonObj = docElement.object();
    QString updateUrl = jsonObj.value("updateUrl").toString();
    if ( updateJson(updateUrl) )
    {
        //Read updated (shutdowns if invalid json file)
        docElement = openJsonFile(DOWNLOADED_PATH);
        //Valid json, replace old one.
        QFile::rename(DOWNLOADED_PATH, FILE_PATH);
        jsonObj = docElement.object();
    }
    QJsonArray repositories = jsonObj.value("repositories").toArray();
    DBG << "repositories size =" << repositories.size() << " updateurl=" << updateUrl;
    QHash<QString, Mod*> modHash;
    for (const QJsonValue& repoVar : repositories)
    {
        QJsonObject repository = repoVar.toObject();
        DBG << "qvariant_cast<QVariantMap>(repoVar);";
        QString name = repository.value("name").toString();
        QString serverAddress = repository.value("serverAddress").toString();
        int serverPort = repository.value("serverPort").toInt();
        QString password = repository.value("password").toString("");
        DBG << "Repo Json parsed";
        Repository* newRepo = new Repository(name, serverAddress, serverPort, password, root);
        DBG << "repo created";
        QJsonArray mods = repository.value("mods").toArray();
        for (const QJsonValue& modVar : mods)
        {
            QJsonObject mod = modVar.toObject();

            QString key = mod.value("key").toString();
            QHash<QString, Mod*>::const_iterator it = modHash.find(key);
            Mod* newMod;
            if (it != modHash.end())
            {
                //Use existing one from another repo.
                newMod = it.value();
            }
            else
            {
                QString modName = mod.value("name").toString();
                bool isOptional =mod.value("optional").toBool(false);
                newMod = new Mod(modName, key, isOptional);
                modHash.insert(key, newMod);
            }
            DBG << "appending mod name =" << newMod->name() << " key =" << newMod->key();
            newRepo->appendMod(newMod); //FIXME: appendChild(newMod) calls TreeItem::appendChild (wtf...)
        }

        DBG << "appending repo name =" << name << " address =" << serverAddress
                 << " port =" << QString::number(serverPort);
        root->appendChild(newRepo);
    }
}
*/
