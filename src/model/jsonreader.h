#ifndef JSONREADER_H
#define JSONREADER_H

#include <QJsonDocument>
#include <QVariantMap>
#include <QString>
#include <QHash>
#include "mod.h"
#include "syncnetworkaccessmanager.h"

class RootItem;
class Repository;

class JsonReader
{
public:
    JsonReader();

    //Fills rootItem according to XML file
    void fillEverything(RootItem* root);
    //Enables better testability.
    void fillEverything(RootItem *root, const QString& jsonFilePath);
    bool updateAvailable();

private:
    static const QString SEPARATOR;

    QVariantMap jsonMap_;
    SyncNetworkAccessManager nam_;
    QString repositoriesPath_; //Holds path to validated repositories.json
    QString downloadedPath_; //Holds path to downloaded but not validated repositories.json

    QJsonDocument readJsonFile(const QString& path) const;
    QVariantMap updateJson(const QString& url);
    QString updateUrl(const QVariantMap& jsonMap) const;
    QHash<QString, Repository*> addedRepos(const RootItem* root) const;
    QSet<QString> addedMods(const Repository* repo) const;
    void removeDeprecatedRepos(RootItem* root, const QSet<QString> jsonRepos);
    void removeDeprecatedMods(Repository* repo, const QSet<QString> jsonMods);
    QHash<QString, Mod*> createModHash(const RootItem* root) const;
};

#endif // JSONREADER_H
