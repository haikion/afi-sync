#include <QCoreApplication>
#include <QFile>
#include <QFileInfo> //Debug prints
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QVariantMap>
#include "afisynclogger.h"
#include "jsonreader.h"
#include "mod.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"
#include "treemodel.h"

const QString JsonReader::SEPARATOR = "|||";

//Fastest
JsonReader::JsonReader(ISync* sync):
    repositoriesPath_(QCoreApplication::applicationDirPath() + "/settings/repositories.json"),
    downloadedPath_(repositoriesPath_ + "_new"),
    sync_(sync)
{
    readJsonFile();
    if (jsonMap_ == QVariantMap())
    {
        LOG_ERROR << "Json file parse failure. Exiting...";
        exit(2);
    }
}

QSet<QString> JsonReader::addedMods(const Repository* repo) const
{
    QSet<QString> rVal;
    for (Mod* mod : repo->mods())
    {
        rVal.insert(mod->key());
    }

    return rVal;
}

void JsonReader::removeDeprecatedMods(Repository* repo, const QSet<QString> jsonMods)
{
    QSet<QString> deprecatedMods = addedMods(repo) - jsonMods;
    for (const QString& key : deprecatedMods)
    {
        repo->removeMod(key);
    }
}

QString JsonReader::updateUrl(const QVariantMap& jsonMap) const
{
    QString updateUrl = qvariant_cast<QString>(jsonMap.value("updateUrl"));
    return updateUrl;
}

bool JsonReader::updateAvailable()
{
    return bytesToJson(fetchJsonBytes(updateUrl(jsonMap_))) != jsonMap_;;
}

QList<Repository*> JsonReader::repositories(ISync* sync)
{
    QList<Repository*> retVal;
    repositories(sync, retVal);
    return retVal;
}

Repository* JsonReader::findRepoByName(const QString& name, QList<Repository*> repositories)
{
    for (Repository* repository : repositories)
    {
        if (repository->name() == name)
        {
            return repository;
        }
    }
    return nullptr;
}

void JsonReader::readJsonFile()
{
    jsonMap_ = qvariant_cast<QVariantMap>(readJsonFile(repositoriesPath_).toVariant());
    const QVariantMap jsonMapUpdate = updateJson(updateUrl(jsonMap_));
    if (jsonMapUpdate != QVariantMap())
        jsonMap_ = jsonMapUpdate;
}

void JsonReader::repositories(ISync* sync, QList<Repository*>& repositories)
{
    readJsonFile();
    const QList<QVariant> jsonRepositories = qvariant_cast<QList<QVariant>>(jsonMap_.value("repositories"));
    QMap<QString, Mod*> modMap; // Add same key mods only once
    for (Repository* repository : repositories)
    {
        for (Mod* mod : repository->mods())
        {
            modMap.insert(mod->key(), mod);
        }
    }
    QSet<QString> jsonKeys;
    for (const QVariant& repoVar : jsonRepositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        QString repoName = qvariant_cast<QString>(repository.value("name"));
        Repository* repo = findRepoByName(repoName, repositories);
        const QString serverAddress = qvariant_cast<QString>(repository.value("serverAddress"));
        const unsigned serverPort = qvariant_cast<unsigned>(repository.value("serverPort"));
        const QString password = qvariant_cast<QString>(repository.value("password", ""));
        if (repo == nullptr)
        {
            repo = new Repository(repoName, serverAddress, serverPort, password, sync_);
        }
        else
        {
            repo->stopUpdates();
            repo->setServerAddress(serverAddress);
            repo->setPort(serverPort);
            repo->setPassword(password);
        }
        repo->setBattlEyeEnabled(qvariant_cast<bool>(repository.value("battlEyeEnabled", true)));

        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
        for (int i = 0; i < mods.size(); ++i)
        {
            const QVariantMap mod = qvariant_cast<QVariantMap>(mods.at(i));
            const QString key = qvariant_cast<QString>(mod.value("key")).toLower();
            jsonKeys.insert(key);
            if (repo->contains(key))
                continue; // Mod is already included in the repository.

            const QString modName = mod.value("name").toString().toLower();
            Mod* newMod = modMap.contains(key) ? modMap.value(key) : new Mod(modName, key, sync);
            modMap.insert(key, newMod); //add if doesn't already exist
            newMod->setFileSize(qvariant_cast<quint64>(mod.value("fileSize", "0")));
            new ModAdapter(newMod, repo, mod.value("optional", false).toBool(), i);
        }
        removeDeprecatedMods(repo, jsonKeys);
        repo->startUpdates();
        if (!repositories.contains(repo))
            repositories.append(repo);
    }
}

QJsonDocument JsonReader::readJsonFile(const QString& path) const
{
    QFile file(path);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        LOG_ERROR << "Failed to open json file: " << path << " file.exists = " << file.exists();
        return QJsonDocument();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
    {
        LOG_ERROR << "Error parsing json file: " << path;
        return QJsonDocument();
    }
    file.close();

    return doc;
}

QString JsonReader::deltaUpdatesKey() const
{
    const QVariantMap jsonMap = qvariant_cast<QVariantMap>(readJsonFile(repositoriesPath_).toVariant());
    return jsonMap.value("deltaUpdates").toString();
}

QByteArray JsonReader::fetchJsonBytes(QString url)
{
    QNetworkReply* reply = nam_.syncGet(QNetworkRequest(url));
    if (reply->bytesAvailable() == 0)
    {
        LOG_WARNING << "Failed. url = " << url;
        return QByteArray();
    }
    QByteArray rVal = reply->readAll();
    reply->deleteLater();
    return rVal;
}

QVariantMap JsonReader::bytesToJson(const QByteArray& bytes) const
{
    QJsonDocument docElement = QJsonDocument::fromJson(bytes);
    if (docElement == QJsonDocument())
    {
        //Incorrect reply from network server
        return QVariantMap();
    }
    return qvariant_cast<QVariantMap>(docElement.toVariant());
}

//Downloads new json file.
QVariantMap JsonReader::updateJson(const QString& url)
{
    QByteArray bytes = fetchJsonBytes(url);
    if (bytes == QByteArray())
    {
        return QVariantMap();
    }
    QFile::remove(repositoriesPath_);
    QFile file(repositoriesPath_);
    if (!file.open(QIODevice::WriteOnly))
    {
        return QVariantMap();
    }
    file.write(bytes);
    file.close();
    return bytesToJson(bytes);
}
