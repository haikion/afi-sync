#ifndef JSONREADER_H
#define JSONREADER_H

#include <QByteArray>
#include <QJsonDocument>
#include <QVariantMap>
#include <QString>
#include <QHash>
#include <QList>
#include <QSet>
#include "mod.h"
#include "apis/isync.h"
#include "syncnetworkaccessmanager.h"

class Repository;

class JsonReader
{
public:
    JsonReader();

    bool updateAvailable();
    QList<Repository*> repositories(ISync* sync);
    QString deltaUpdatesKey();
    QSet<QString> getRemovables(QList<Repository *>& updateRepositories);
    QSet<QString> updateRepositories(ISync* sync, QList<Repository*>& updateRepositories);

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
    QSet<QString> getRemovablesOffline(QList<Repository*>& updatedRepositories) const;
    QSet<QString> updateRepositoriesOffline(ISync* sync, QList<Repository*>& updateRepositories);
};

#endif // JSONREADER_H
