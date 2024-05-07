#include "jsonreader.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QSharedDataPointer>
#include <QStringLiteral>
#include <QVariantMap>

#include "afisynclogger.h"
#include "fileutils.h"
#include "jsonutils.h"
#include "mod.h"
#include "modadapter.h"
#include "repository.h"

using namespace Qt::StringLiterals;

const QString JsonReader::JSON_RELATIVE_PATH = u"/settings/repositories.json"_s;

void JsonReader::readJson()
{
    if (!readJsonFile())
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
    if (url.isEmpty())
    {
        return false; // No update url set
    }
    const QByteArray bytes = nam_.fetchBytes(url);
    if (bytes.isEmpty())
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

void JsonReader::updateRepositoriesOffline(ISync* sync, QList<QSharedPointer<Repository>>& repositories)
{
    const QList<QVariant> jsonRepositories = jsonMap_.value(u"repositories"_s).toList();
    QSet<QString> previousModKeys;
    QMap<QString, QSharedPointer<Mod>> modMap; // Add same key mods only once
    for (const auto& repository : repositories)
    {
        for (const auto& mod : repository->mods())
        {
            modMap.insert(mod->key(), mod);
            previousModKeys.insert(mod->key());
        }
    }
    sync->setDeltaUrls(jsonMap_.value(u"deltaUpdates2"_s).toStringList());
    QHash<Repository*, QSet<QString>> repositoryJsonModKeys;
    QList<QSharedPointer<Repository>> orderedRepositoryList;
    for (const QVariant& repoVar : jsonRepositories)
    {
        auto repository = repoVar.toMap();
        auto repoName = repository.value(u"name"_s).toString();
        auto repo = Repository::findRepoByName(repoName, repositories);
        const auto serverAddress = repository.value(u"serverAddress"_s).toString();
        const auto serverPort = qvariant_cast<unsigned>(repository.value(u"serverPort"_s));
        const auto password = repository.value(u"password"_s, "").toString();
        if (repo == nullptr)
        {
            repo = QSharedPointer<Repository>(new Repository(repoName, serverAddress, serverPort, password));
        }
        else
        {
            repo->stopUpdates();
            repo->setServerAddress(serverAddress);
            repo->setPort(serverPort);
            repo->setPassword(password);
        }
        repo->setBattlEyeEnabled(repository.value(u"battlEyeEnabled"_s, true).toBool());

        const auto mods = repository.value(u"mods"_s).toList();
        QSet<QString> jsonModKeys;
        for (int i = 0; i < mods.size(); ++i)
        {
            const auto mod = mods.at(i).toMap();
            const auto key = mod.value(u"key"_s).toString().toLower();
            jsonModKeys.insert(key);
            if (repo->contains(key))
                continue; // Mod is already included in the repository.

            const auto modName = mod.value(u"name"_s).toString().toLower();
            auto newMod = modMap.contains(key) ? modMap.value(key) :
                              QSharedPointer<Mod>(new Mod(modName, key, sync), &QObject::deleteLater);
            modMap.insert(key, newMod); //add if doesn't already exist
            newMod->setFileSize(qvariant_cast<quint64>(mod.value(u"fileSize"_s, "0")));
            new ModAdapter(newMod, repo.get(), mod.value(u"optional"_s, false).toBool(), i);
        }
        repositoryJsonModKeys.insert(repo.get(), jsonModKeys);
        orderedRepositoryList.append(repo);
    }

    // First add all mods and then remove deprecated in order to not
    // destroy moved mods.
    const auto& repos = repositoryJsonModKeys.keys();
    for (Repository* repo: repos)
    {
        repo->removeDeprecatedMods(repositoryJsonModKeys.value(repo));
        repo->startUpdates();
    }
    repositories.clear();
    repositories.append(orderedRepositoryList);
}

QSet<QString> JsonReader::getRemovables(const QList<QSharedPointer<Repository>>& repositories)
{
    updateJsonMap();
    return getRemovablesOffline(repositories);
}

QSet<QString> JsonReader::getRemovablesOffline(const QList<QSharedPointer<Repository>>& repositories) const
{
    const auto jsonRepositories = jsonMap_.value(u"repositories"_s).toList();
    QSet<QString> previousModKeys;
    for (const auto& repository : repositories)
    {
        for (const auto& mod : repository->mods())
        {
            previousModKeys.insert(mod->key());
        }
    }
    QSet<QString> jsonModKeys;
    for (const QVariant& repoVar : jsonRepositories)
    {
        auto repository = repoVar.toMap();
        const auto mods = repository.value(u"mods"_s).toList();
        for (const auto& modObj : mods)
        {
            auto mod = modObj.toMap();
            auto key = mod.value(u"key"_s).toString().toLower();
            jsonModKeys.insert(key);
        }
    }
    return previousModKeys - jsonModKeys;
}

// Updates repository list and returns set of deleted mod keys
void JsonReader::updateRepositories(ISync* sync, QList<QSharedPointer<Repository>>& repositories)
{
    updateJsonMap();
    updateRepositoriesOffline(sync, repositories);
}

QStringList JsonReader::deltaUrls()
{
    updateJsonMap();
    return jsonMap_.value(u"deltaUpdates2"_s).toStringList();
}

bool JsonReader::writeJsonBytes(const QByteArray& data)
{
    return FileUtils::writeFile(data, QCoreApplication::applicationDirPath() + JSON_RELATIVE_PATH);
}

bool JsonReader::updateJsonMap()
{
    const QString url = updateUrl();
    if (url.isEmpty())
    {
        return false;
    }
    const QByteArray bytes = nam_.fetchBytes(url);
    const QVariantMap jsonMapUpdate = JsonUtils::bytesToJsonMap(bytes);
    if (!jsonMapUpdate.isEmpty())
    {
        jsonMap_ = jsonMapUpdate;
        return writeJsonBytes(bytes);
    }
    return false;
}
