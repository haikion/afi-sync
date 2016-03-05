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
