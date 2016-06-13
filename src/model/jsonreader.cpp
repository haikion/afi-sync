#include <QFile>
#include <QFileInfo> //Debug prints
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
const QString JsonReader::SEPARATOR = "|||";
QVariantMap JsonReader::jsonMap_;
SyncNetworkAccessManager JsonReader::nam_;


//Fastest
void JsonReader::fillEverything(RootItem* root)
{
    fillEverything(root, FILE_PATH);
}

void JsonReader::fillEverything(RootItem* root, const QString& jsonFilePath)
{
    DBG << "Stopping UI updates.";
    bool updateRunning = root->stopUpdates();
    QJsonDocument origDoc = readJsonFile(jsonFilePath);
    jsonMap_ = qvariant_cast<QVariantMap>(origDoc.toVariant());
    QString updateUrlStr = updateUrl(jsonMap_);
    QVariantMap jsonMapUpdate = updateJson(updateUrlStr);
    if ( jsonMapUpdate != QVariantMap() )
    {
        jsonMap_ = jsonMapUpdate;
    }
    if (jsonMap_ == QVariantMap())
    {
        DBG << "ERROR: Json file parse failure. Exiting...";
        exit(2);
    }
    QList<QVariant> repositories = qvariant_cast<QList<QVariant>>(jsonMap_.value("repositories"));
    DBG << "repositories size =" << repositories.size() << "updateurl =" << updateUrlStr;
    QHash<QString, Mod*> modHash;
    QSet<QString> jsonMods; //Holds mod + repo listing of mods that are in repositories.json document.
    QSet<QString> jsonRepos;
    QHash<QString, Repository*> adRepos = addedRepos(root);
    for (const QVariant& repoVar : repositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        QString repoName = qvariant_cast<QString>(repository.value("name"));
        jsonRepos.insert(repoName);
        DBG << "Repo Json parsed";
        Repository* repo;
        auto it = adRepos.find(repoName);
        if (it != adRepos.end())
        {
            DBG << "Repo" << repoName << "already in root. Using it";
            repo = it.value();
        }
        else
        {
            QString serverAddress = qvariant_cast<QString>(repository.value("serverAddress"));
            unsigned serverPort = qvariant_cast<unsigned>(repository.value("serverPort"));
            QString password = qvariant_cast<QString>(repository.value("password", ""));
            repo = new Repository(repoName, serverAddress, serverPort, password, root);
            DBG << "appending repo name =" << repoName << " address =" << serverAddress
                << " port =" << QString::number(serverPort);
            root->appendChild(repo);
        }
        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
        for (QVariant modVar : mods)
        {
            QVariantMap mod = qvariant_cast<QVariantMap>(modVar);
            QString key = qvariant_cast<QString>(mod.value("key")).toLower();
            DBG << "key parsed. key =" << key;
            //Do not add same mod to same repo twice.
            jsonMods.insert(key);
            if (repo->contains(key))
            {
                DBG << "Key" << key << "already in" << repoName;
                continue;
            }
            QHash<QString, Mod*>::const_iterator it = modHash.find(key);
            Mod* newMod;
            if (it != modHash.end())
            {
                DBG << "Already added. Using existing value.";
                //Use existing one from another repo.
                newMod = it.value();
            }
            else
            {
                DBG << "Creating new mod...";
                QString modName = qvariant_cast<QString>(mod.value("name"));
                bool isOptional = qvariant_cast<bool>(mod.value("optional", false));
                DBG << "Parsed mod parameters";
                newMod = new Mod(modName, key.toLower(), isOptional);
                DBG << "New mod object created.";
                modHash.insert(key, newMod);
                DBG << "added to modhash";
            }
            DBG << "appending mod name =" << newMod->name() << " key =" << newMod->key();
            repo->appendMod(newMod);
        }
    }

    DBG << "Checking deprecated mods...";
    QSet<QString> adMods = addedMods(root);
    QSet<QString> depreMod = adMods - jsonMods;
    for (const QString& dm : depreMod)
    {
        DBG << "Removing deprecated mod:" << dm << root->childCount();
        for (Repository* repo : root->childItems())
        {
            repo->removeMod(dm);
        }
    }
    QSet<QString> depreRepos = adRepos.keys().toSet() - jsonRepos;
    for (const QString& dr : depreRepos)
    {
        DBG << "Removing deprecated repo:" << dr;
        Repository* repo = adRepos.value(dr);
        root->removeChild(repo);
        delete repo;
    }
    DBG << "Starting UI updates.";
    if (updateRunning)
    {
        root->startUpdates();
    }
}

QHash<QString, Repository*> JsonReader::addedRepos(RootItem* root)
{
    QHash<QString, Repository*> rVal;
    for (Repository* repo : root->childItems())
    {
        rVal.insert(repo->name(), repo);
    }
    return rVal;
}

QSet<QString> JsonReader::addedMods(RootItem* root)
{
    QSet<QString> rVal;
    for (Repository* repo : root->childItems())
    {
        for (Mod* mod : repo->mods())
        {
            rVal.insert(mod->key().toLower());
        }
    }
    return rVal;
}

QString JsonReader::updateUrl(const QVariantMap& jsonMap)
{
    QString updateUrl = qvariant_cast<QString>(jsonMap.value("updateUrl"));
    return updateUrl;
}

bool JsonReader::updateAvaible()
{
    QVariantMap jsonMapUp = updateJson(updateUrl(jsonMap_));
    if (jsonMapUp != QVariantMap() && jsonMapUp != jsonMap_)
    {
        return true;
    }
    return false;
}

QJsonDocument JsonReader::readJsonFile(const QString& path)
{
    //QString absolutePath = QCoreApplication::applicationFilePath() + "/" +path;
    QFile file(path);
    DBG << "opening file. path =" << QFileInfo(path).absoluteFilePath();
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        DBG << "ERROR: failed to open json file: " << path << " file.exists =" << file.exists();
        return QJsonDocument();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
    {
        DBG << "ERROR: error parsing json file: " << path;
        return QJsonDocument();
    }
    file.close();

    return doc;
}

//Downloads new json file.
QVariantMap JsonReader::updateJson(const QString& url)
{
    QNetworkReply* reply = nam_.syncGet(QNetworkRequest(url));
    QFile file(DOWNLOADED_PATH);
    if (reply->bytesAvailable() == 0 || !file.open(QIODevice::WriteOnly))
    {
        DBG << "failed. url =" << url;
        return QVariantMap();
    }
    file.write(reply->readAll());
    file.close();
    reply->deleteLater();

    //Read updated (shutdowns if invalid json file)
    QJsonDocument docElement = readJsonFile(DOWNLOADED_PATH);
    if (docElement == QJsonDocument())
    {
        return QVariantMap();
    }
    QFile::remove(FILE_PATH);
    //Valid json, replace old one.
    QFile::rename(DOWNLOADED_PATH, FILE_PATH);
    QVariantMap jsonMap = qvariant_cast<QVariantMap>(docElement.toVariant());
    return jsonMap;
}
