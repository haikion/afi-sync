#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QStringLiteral>
#include <QVariantMap>

#include "afisync.h"
#include "afisynclogger.h"
#include "fileutils.h"
#include "jsonreader.h"
#include "jsonutils.h"
#include "mod.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"
#include "settingsmodel.h"

using namespace Qt::StringLiterals;

const QString JsonReader::JSON_RELATIVE_PATH = u"/settings/repositories.json"_s;

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
    updateRepositoriesOffline(sync, retVal);
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

// TODO: No need to remove mods anymore, instead just add them if needed
QSet<QString> JsonReader::updateRepositoriesOffline(ISync* sync, QList<Repository*>& repositories)
{
    const QString deltaUpdatesUrl = qvariant_cast<QString>(
        jsonMap_.value(u"deltaUpdates"_s));
    if (SettingsModel::deltaPatchingEnabled() && !deltaUpdatesUrl.isEmpty()) {
        sync->setDeltaUpdatesFolder(deltaUpdatesUrl);
    } else {
        sync->setDeltaUpdatesFolder({});
    }
    const QList<QVariant> jsonRepositories = qvariant_cast<QList<QVariant>>(
        jsonMap_.value(u"repositories"_s));
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
        QString repoName = qvariant_cast<QString>(repository.value(u"name"_s));
        Repository* repo = Repository::findRepoByName(repoName, repositories);
        const QString serverAddress = qvariant_cast<QString>(
            repository.value(u"serverAddress"_s));
        const unsigned serverPort = qvariant_cast<unsigned>(
            repository.value(u"serverPort"_s));
        const QString password = qvariant_cast<QString>(
            repository.value(u"password"_s, ""));
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
        repo->setBattlEyeEnabled(
            qvariant_cast<bool>(repository.value(u"battlEyeEnabled"_s, true)));

        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(
            repository.value(u"mods"_s));
        QSet<QString> jsonModKeys;
        for (int i = 0; i < mods.size(); ++i)
        {
            const QVariantMap mod = qvariant_cast<QVariantMap>(mods.at(i));
            const QString key = qvariant_cast<QString>(mod.value(u"key"_s)).toLower();
            jsonModKeys.insert(key);
            if (repo->contains(key))
                continue; // Mod is already included in the repository.

            const QString modName = mod.value(u"name"_s).toString().toLower();
            Mod* newMod = modMap.contains(key) ? modMap.value(key) : new Mod(modName, key, sync);
            modMap.insert(key, newMod); //add if doesn't already exist
            newMod->setFileSize(qvariant_cast<quint64>(mod.value(u"fileSize"_s, "0")));
            new ModAdapter(newMod, repo, mod.value(u"optional"_s, false).toBool(), i);
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

QSet<QString> JsonReader::getRemovables(QList<Repository*>& repositories)
{
    updateJsonMap();
    return getRemovablesOffline(repositories);
}

QSet<QString> JsonReader::getRemovablesOffline(QList<Repository*>& repositories) const
{
    const QList<QVariant> jsonRepositories = qvariant_cast<QList<QVariant>>(
        jsonMap_.value(u"repositories"_s));
    QSet<QString> previousModKeys;
    for (Repository* repository : repositories)
    {
        for (Mod* mod : repository->mods())
        {
            previousModKeys.insert(mod->key());
        }
    }
    QSet<QString> jsonModKeys;
    for (const QVariant& repoVar : jsonRepositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(
            repository.value(u"mods"_s));
        for (int i = 0; i < mods.size(); ++i)
        {
            const QVariantMap mod = qvariant_cast<QVariantMap>(mods.at(i));
            const QString key = qvariant_cast<QString>(mod.value(u"key"_s)).toLower();
            jsonModKeys.insert(key);
        }
    }
    return previousModKeys - jsonModKeys;
}

// Updates repository list and returns set of deleted mod keys
QSet<QString> JsonReader::updateRepositories(ISync* sync, QList<Repository*>& repositories)
{
    updateJsonMap();
    return updateRepositoriesOffline(sync, repositories);
}

QString JsonReader::deltaUpdatesKey()
{
    updateJsonMap();
    return jsonMap_.value(u"deltaUpdates"_s).toString();
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
