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
    [[nodiscard]] QList<Repository*> repositories(ISync* sync);
    [[nodiscard]] QStringList deltaUrls();
    [[nodiscard]] QSet<QString> getRemovables(const QList<Repository *>& updateRepositories);
    void updateRepositories(ISync* sync, QList<Repository*>& updateRepositories);

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
    QSet<QString> getRemovablesOffline(const QList<Repository*>& updatedRepositories) const;
    void updateRepositoriesOffline(ISync* sync, QList<Repository*>& repositories);
};

#endif // JSONREADER_H
