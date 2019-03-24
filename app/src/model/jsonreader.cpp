#include <QCoreApplication>
#include <QFile>
#include <QFileInfo> //Debug prints
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QVariantMap>
#include "afisync.h"
#include "afisynclogger.h"
#include "jsonreader.h"
#include "mod.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"
#include "treemodel.h"
#include "fileutils.h"
#include "jsonutils.h"

const QString JsonReader::JSON_RELATIVE_PATH = "/settings/repositories.json";

JsonReader::JsonReader()
{
    if (!readJsonFile())
    {
        LOG_ERROR << "Json file parse failure. Exiting...";
        exit(2);
    }
    nam_ = std::make_unique<SyncNetworkAccessManager>();
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
    const QByteArray bytes = nam_->fetchBytes(url);
    if (bytes == QByteArray())
    {
        return false; // Unresponsive url
    }
    const QVariantMap map = JsonUtils::bytesToJsonMap(bytes);
    if (map.isEmpty())
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

bool JsonReader::readJsonFile()
{
    const QByteArray jsonBytes = readJsonBytes();
    QVariantMap jsonMap = JsonUtils::bytesToJsonMap(jsonBytes);
    if (jsonMap.isEmpty())
    {
        return false;
    }
    jsonMap_ = jsonMap;
    return true;
}

QByteArray JsonReader::readJsonBytes() const
{
    return FileUtils::readFile(QCoreApplication::applicationDirPath() + JSON_RELATIVE_PATH);
}

void JsonReader::setSyncNetworkAccessManager(std::unique_ptr<SyncNetworkAccessManager> syncNetworkAccessManager)
{
    nam_ = std::move(syncNetworkAccessManager);
}

QSet<QString> JsonReader::updateRepositoriesOffline(ISync* sync, QList<Repository*>& repositories)
{
    const QList<QVariant> jsonRepositories = qvariant_cast<QList<QVariant>>(jsonMap_.value("repositories"));
    QSet<QString> previousModKeys;
    QMap<QString, Mod*> modMap; // Add same key mods only once
    for (Repository* repository : repositories)
    {
        for (Mod* mod : repository->mods())
        {
            modMap.insert(mod->key(), mod);
            previousModKeys.insert(mod->key());
        }
    }
    QHash<Repository*, QSet<QString>> repositoryJsonModKeys;
    QList<Repository*> orderedRepositoryList;
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
            repo = new Repository(repoName, serverAddress, serverPort, password);
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
        orderedRepositoryList.append(repo);
    }

    // First add all mods and then remove deprecated in order to not
    // destroy moved mods.
    for (Repository* repo: repositoryJsonModKeys.keys())
    {
        repo->removeDeprecatedMods(repositoryJsonModKeys.value(repo));
        repo->startUpdates();
    }
    repositories.clear();
    repositories.append(orderedRepositoryList);
    return previousModKeys - AfiSync::combine(repositoryJsonModKeys.values());
}

// Updates repository list and returns set of deleted mod keys
QSet<QString> JsonReader::updateRepositories(ISync* sync, QList<Repository*>& repositories)
{
    updateJsonMap();
    return updateRepositoriesOffline(sync, repositories);
}

QString JsonReader::deltaUpdatesKey() const
{
    return jsonMap_.value("deltaUpdates").toString();
}

bool JsonReader::writeJsonBytes(const QByteArray& data)
{
    return FileUtils::writeFile(data, QCoreApplication::applicationDirPath() + JSON_RELATIVE_PATH);
}

bool JsonReader::updateJsonMap()
{
    const QString url = updateUrl();
    if (url == QString())
    {
        return false;
    }
    const QByteArray bytes = nam_->fetchBytes(url);
    const QVariantMap jsonMapUpdate = JsonUtils::bytesToJsonMap(bytes);
    if (!jsonMapUpdate.isEmpty())
    {
        jsonMap_ = jsonMapUpdate;
        return writeJsonBytes(bytes);
    }
    return false;
}
