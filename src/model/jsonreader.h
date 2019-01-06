#ifndef JSONREADER_H
#define JSONREADER_H

#include <QJsonDocument>
#include <QVariantMap>
#include <QString>
#include <QHash>
#include <QList>
#include "mod.h"
#include "syncnetworkaccessmanager.h"
#include "interfaces/irepository.h"

class RootItem;
class Repository;

class JsonReader
{
public:
    JsonReader(ISync* sync = nullptr);

    //Fills rootItem according to XML file
    void fillEverything(RootItem* root);
    //Enables better testability.
    void fillEverything(RootItem *root, const QString& jsonFilePath);
    bool updateAvailable();
    QList<Repository*> repositories(ISync* sync);
    QString deltaUpdatesKey() const;    
    void repositories(ISync* sync, QList<Repository*>& repositories);

private:
    static const QString SEPARATOR;

    QVariantMap jsonMap_;
    SyncNetworkAccessManager nam_;
    QString repositoriesPath_; //Holds path to validated repositories.json
    QString downloadedPath_; //Holds path to downloaded but not validated repositories.json
    ISync* sync_;

    QJsonDocument readJsonFile(const QString& path) const;
    QVariantMap updateJson(const QString& url);
    QString updateUrl(const QVariantMap& jsonMap) const;
    QSet<QString> addedMods(const Repository* repo) const;
    void removeDeprecatedRepos(QList<Repository*>& repositoriest, const QSet<QString> jsonRepos);
    void removeDeprecatedMods(Repository* repo, const QSet<QString> jsonMods);
    QByteArray fetchJsonBytes(QString url);
    QVariantMap bytesToJson(const QByteArray& bytes) const;
    static Repository* findRepoByName(const QString& name, QList<Repository*> repositories);
    void readJsonFile();
};

#endif // JSONREADER_H
