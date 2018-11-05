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
#include "rootitem.h"
#include "settingsmodel.h"

const QString JsonReader::SEPARATOR = "|||";

//Fastest
JsonReader::JsonReader(ISync* sync):
    repositoriesPath_(QCoreApplication::applicationDirPath() + "/settings/repositories.json"),
    downloadedPath_(repositoriesPath_ + "_new"),
    sync_(sync)
{}

QHash<QString, Repository*> JsonReader::addedRepos(const RootItem* root) const
{
    QHash<QString, Repository*> rVal;
    for (Repository* repo : root->childItems())
    {
        rVal.insert(repo->name(), repo);
    }
    return rVal;
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

//Creates data struture for added mods.
QHash<QString, Mod*> JsonReader::createModHash(const RootItem* root) const
{
    QHash<QString, Mod*> rVal;
    for (Repository* repo : root->childItems())
    {
        for (Mod* mod : repo->mods())
        {
            rVal.insert(mod->key(), mod);
        }
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

void JsonReader::removeDeprecatedRepos(RootItem* root, const QSet<QString> jsonRepos)
{
    LOG << "Checking deprecated mods...";

    QHash<QString, Repository*> adRepos = addedRepos(root);
    QSet<QString> depreRepos = adRepos.keys().toSet() - jsonRepos;
    for (const QString& repoName : depreRepos)
    {
        LOG << "Removing deprecated repo: " << repoName;
        Repository* repo = adRepos.value(repoName);
        root->removeChild(repo);
        delete repo;
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
    jsonMap_ = qvariant_cast<QVariantMap>(readJsonFile(repositoriesPath_).toVariant());
    const QVariantMap jsonMapUpdate = updateJson(updateUrl(jsonMap_));
    if (jsonMapUpdate != QVariantMap())
        jsonMap_ = jsonMapUpdate;

    if (jsonMap_ == QVariantMap())
    {
        LOG_ERROR << "Json file parse failure. Exiting...";
        exit(2);
    }

    const QList<QVariant> repositories = qvariant_cast<QList<QVariant>>(jsonMap_.value("repositories"));
    QMap<QString, Mod*> modMap; // Add same key mods only once
    for (const QVariant& repoVar : repositories)
    {
        QVariantMap repository = qvariant_cast<QVariantMap>(repoVar);
        QString repoName = qvariant_cast<QString>(repository.value("name"));
        Repository* repo;
        const QString serverAddress = qvariant_cast<QString>(repository.value("serverAddress"));
        const unsigned serverPort = qvariant_cast<unsigned>(repository.value("serverPort"));
        const QString password = qvariant_cast<QString>(repository.value("password", ""));
        repo = new Repository(repoName, serverAddress, serverPort, password, sync_);
        repo->setBattlEyeEnabled(qvariant_cast<bool>(repository.value("battlEyeEnabled", true)));

        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
        for (int i = 0; i < mods.size(); ++i)
        {
            const QVariantMap mod = qvariant_cast<QVariantMap>(mods.at(i));
            const QString key = qvariant_cast<QString>(mod.value("key")).toLower();
            LOG << "key parsed. key = " << key;
            if (repo->contains(key))
            {
                LOG << "Key " << key << " already in " << repoName;
                continue;
            }

            const QString modName = mod.value("name").toString().toLower();
            Mod* newMod = modMap.contains(key) ? modMap.value(key) : new Mod(modName, key, sync);
            modMap.insert(key, newMod); //add if doesn't already exist
            newMod->setFileSize(qvariant_cast<quint64>(mod.value("fileSize", "0")));
            new ModAdapter(newMod, repo, mod.value("optional", false).toBool(), i);
        }
        repo->startUpdates();
        retVal.append(repo);
    }

    return retVal;
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
    return qvariant_cast<QString>(jsonMap_.value("deltaUpdates"));
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
