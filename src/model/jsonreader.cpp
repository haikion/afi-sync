#include <QFile>
#include <QFileInfo> //Debug prints
#include <QNetworkReply>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include "mod.h"
#include "repository.h"
#include "rootitem.h"
#include "jsonreader.h"
#include "debug.h"
#include "modadapter.h"
#include "settingsmodel.h"

const QString JsonReader::FILE_PATH = "settings/repositories.json";
const QString JsonReader::DOWNLOADED_PATH = FILE_PATH + "_new";
const QString JsonReader::SEPARATOR = "|||";

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
    //Delta update
    QString newDeltaUpdateUrl = jsonMap_.value("deltaUpdates").toString();
    if (SettingsModel::deltaPatchingEnabled()
            && newDeltaUpdateUrl.size() != 0 && root->deltaUpdatesKey() != newDeltaUpdateUrl)
    {
        DBG << "Updating delta update url";
        root->setDeltaUpdatesKey(newDeltaUpdateUrl);
    }

    QList<QVariant> repositories = qvariant_cast<QList<QVariant>>(jsonMap_.value("repositories"));
    DBG << "repositories size =" << repositories.size() << "updateurl =" << updateUrlStr;
    QHash<QString, Mod*> modHash = createModHash(root);
    QHash<Repository*, QSet<QString>> jsonMods; //Holds mod + repo listing of mods that are in repositories.json document.
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
        bool battlEyeEnabled = qvariant_cast<bool>(repository.value("battlEyeEnabled", true));
        repo->setBattlEyeEnabled(battlEyeEnabled);
        QSet<QString> jsonMods1;
        QList<QVariant> mods = qvariant_cast<QList<QVariant>>(repository.value("mods"));
        for (int i = 0; i < mods.size(); ++i)
        {
            QVariant modVar = mods.at(i);
            QVariantMap mod = qvariant_cast<QVariantMap>(modVar);
            QString key = qvariant_cast<QString>(mod.value("key")).toLower();
            DBG << "key parsed. key =" << key;
            //Do not add same mod to same repo twice.
            jsonMods1.insert(key);
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
                QString modName = qvariant_cast<QString>(mod.value("name"));
                DBG << "Parsed mod parameters for" << modName;
                newMod = new Mod(modName, key.toLower());
                DBG << "New mod object created:" << modName;
                newMod->setFileSize(qvariant_cast<quint64>(mod.value("fileSize", "0")));
                modHash.insert(key, newMod);
                DBG << modName << "added to modhash.";
            }
            bool isOptional = mod.value("optional", false).toBool();
            DBG << "Appending mod (name =" << newMod->name()
                << " key =" << newMod->key()
                << ") to repository" << repoName;
            new ModAdapter(newMod, repo, isOptional, i);
        }
        jsonMods.insert(repo, jsonMods1);
    }

    removeDeprecatedRepos(root, jsonRepos);

    for (Repository* repo : root->childItems())
        removeDeprecatedMods(repo, jsonMods.value(repo));

    DBG << "Starting UI updates.";
    if (updateRunning)
        root->startUpdates();

}

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
        rVal.insert(mod->key().toLower());
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
            rVal.insert(mod->key().toLower(), mod);
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
    DBG << "Checking deprecated mods...";

    QHash<QString, Repository*> adRepos = addedRepos(root);
    QSet<QString> depreRepos = adRepos.keys().toSet() - jsonRepos;
    for (const QString& repoName : depreRepos)
    {
        DBG << "Removing deprecated repo:" << repoName;
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
    QVariantMap jsonMapUp = updateJson(updateUrl(jsonMap_));
    if (jsonMapUp != QVariantMap() && jsonMapUp != jsonMap_)
    {
        return true;
    }
    return false;
}

QJsonDocument JsonReader::readJsonFile(const QString& path) const
{
    QFile file(path);
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
