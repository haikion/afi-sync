#ifndef JSONREADER_H
#define JSONREADER_H

#include <memory>
#include <QByteArray>
#include <QJsonDocument>
#include <QVariantMap>
#include <QString>
#include <QHash>
#include <QList>
#include <QSet>
#include "mod.h"
#include "syncnetworkaccessmanager.h"
#include "interfaces/irepository.h"

class Repository;

class JsonReader
{
public:
    JsonReader();

    bool updateAvailable();
    QList<Repository*> repositories(std::shared_ptr<ISync> sync);
    QString deltaUpdatesKey() const;    
    QSet<QString> updateRepositories(std::shared_ptr<ISync> sync, QList<Repository*>& updateRepositories);

protected:
    virtual bool writeJsonBytes(const QByteArray& bytes);
    virtual QByteArray readJsonBytes() const;
    void setSyncNetworkAccessManager(std::unique_ptr<SyncNetworkAccessManager> syncNetworkAccessManager);

private:
    static const QString JSON_RELATIVE_PATH;

    QVariantMap jsonMap_;
    std::unique_ptr<SyncNetworkAccessManager> nam_;

    bool updateJsonMap();
    QString updateUrl(const QVariantMap& jsonMap) const;
    QString updateUrl() const;
    QByteArray fetchJsonBytes(QString url);
    bool readJsonFile();
    QSet<QString> updateRepositoriesOffline(std::shared_ptr<ISync> sync, QList<Repository*>& updateRepositories);
};

#endif // JSONREADER_H
