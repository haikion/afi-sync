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
#include "fileutils.h"
#include "jsonutils.h"

const QString JsonReader::SEPARATOR = "|||";

// Fastest
JsonReader::JsonReader():
    repositoriesPath_(QCoreApplication::applicationDirPath() + "/settings/repositories.json"),
    downloadedPath_(repositoriesPath_ + "_new")
{
    readJsonFile();
    if (jsonMap_ == QVariantMap())
    {
        LOG_ERROR << "Json file parse failure. Exiting...";
        exit(2);
    }
}

QString JsonReader::updateUrl() const
{
    return JsonUtils::updateUrl(jsonMap_);
}

bool JsonReader::updateAvailable()
{
    const QString url = updateUrl();
    if (url == QString())
    {
        return false; // No update url set
    }
    const QByteArray bytes = nam_.fetchBytes(url);
    if (bytes == QByteArray())
    {
        return false; // Unresponsive url
    }
    const QVariantMap map = JsonUtils::bytesToJsonMap(bytes);
    if (map == QVariantMap())
    {
        return false; // Data is not json
    }
    return JsonUtils::bytesToJsonMap(bytes) != jsonMap_;
}

QList<Repository*> JsonReader::repositories(ISync* sync)
{
    QList<Repository*> retVal;
    updateRepositories(sync, retVal);
    return retVal;
}

void JsonReader::readJsonFile()
{
    const QByteArray jsonBytes = FileUtils::readFile(repositoriesPath_);
    jsonMap_ = JsonUtils::bytesToJsonMap(jsonBytes);
    if (jsonMap_ == QVariantMap())
    {
        LOG_ERROR << "Failed to open json file: " << repositoriesPath_;
        exit(2);
    }
}

void JsonReader::updateRepositoriesOffline(ISync* sync, QList<Repository*>& repositories)
{
    const QList<QVariant> jsonRepositories = qvariant_cast<QList<QVariant>>(jsonMap_.value("repositories"));
    QMap<QString, Mod*> modMap; // Add same key mods only once
    for (Repository* repository : repositories)
    {
        for (Mod* mod : repository->mods())
        {
            modMap.insert(mod->key(), mod);
        }
    }
    QMap<Repository*, QSet<QString>> repositoryJsonModKeys;
    for (const QVariant& repoVar : jsonRepositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        QString repoName = qvariant_cast<QString>(repository.value("name"));
        Repository* repo = Repository::findRepoByName(repoName, repositories);
        const QString serverAddress = qvariant_cast<QString>(repository.value("serverAddress"));
        const unsigned serverPort = qvariant_cast<unsigned>(repository.value("serverPort"));
        const QString password = qvariant_cast<QString>(repository.value("password", ""));
        if (repo == nullptr)
        {
            repo = new Repository(repoName, serverAddress, serverPort, password, sync);
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
        QSet<QString> jsonModKeys;
        for (int i = 0; i < mods.size(); ++i)
        {
            const QVariantMap mod = qvariant_cast<QVariantMap>(mods.at(i));
            const QString key = qvariant_cast<QString>(mod.value("key")).toLower();
            jsonModKeys.insert(key);
            if (repo->contains(key))
                continue; // Mod is already included in the repository.

            const QString modName = mod.value("name").toString().toLower();
            Mod* newMod = modMap.contains(key) ? modMap.value(key) : new Mod(modName, key, sync);
            modMap.insert(key, newMod); //add if doesn't already exist
            newMod->setFileSize(qvariant_cast<quint64>(mod.value("fileSize", "0")));
            new ModAdapter(newMod, repo, mod.value("optional", false).toBool(), i);
        }
        repositoryJsonModKeys.insert(repo, jsonModKeys);
        if (!repositories.contains(repo))
            repositories.append(repo);
    }

    // First add all mods and then remove deprecated in order to not
    // destroy moved mods.
    for (Repository* repo : repositoryJsonModKeys.keys())
    {
        repo->removeDeprecatedMods(repositoryJsonModKeys.value(repo));
        repo->startUpdates();
    }
}

void JsonReader::updateRepositories(ISync* sync, QList<Repository*>& repositories)
{
    updateJsonMap();
    updateRepositoriesOffline(sync, repositories);
}

QString JsonReader::deltaUpdatesKey() const
{
    return jsonMap_.value("deltaUpdates").toString();
}

bool JsonReader::updateJsonMap()
{
    const QString url = updateUrl();
    if (url == QString())
    {
        return false;
    }
    const QByteArray bytes = nam_.fetchBytes(url);
    const QVariantMap jsonMapUpdate = JsonUtils::bytesToJsonMap(bytes);
    if (jsonMapUpdate != QVariantMap())
    {
        jsonMap_ = jsonMapUpdate;
        return true;
    }
    return false;
}
