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

class Repository;

class JsonReader
{
public:
    JsonReader();

    bool updateAvailable();
    QList<Repository*> repositories(ISync* sync);
    QString deltaUpdatesKey() const;    
    void updateRepositories(ISync* sync, QList<Repository*>& updateRepositories);

private:
    static const QString SEPARATOR;

    QVariantMap jsonMap_;
    SyncNetworkAccessManager nam_;
    QString repositoriesPath_; //Holds path to validated repositories.json
    QString downloadedPath_; //Holds path to downloaded but not validated repositories.json

    bool updateJsonMap();
    QString updateUrl(const QVariantMap& jsonMap) const;
    QString updateUrl() const;
    void removeDeprecatedRepos(QList<Repository*>& repositoriest, const QSet<QString> jsonRepos);
    QByteArray fetchJsonBytes(QString url);
    void readJsonFile();
    void updateRepositoriesOffline(ISync* sync, QList<Repository*>& updateRepositories);
};

#endif // JSONREADER_H
