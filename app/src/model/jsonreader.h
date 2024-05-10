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
    JsonReader() = default;
    virtual ~JsonReader() = default;

    void readJson();
    bool updateAvailable();
    [[nodiscard]] QList<Repository*> repositories(ISync* sync);
    [[nodiscard]] QStringList deltaUrls();
    [[nodiscard]] QSet<QString> getRemovables(const QList<QSharedPointer<Repository>>& updateRepositories);
    void updateRepositories(LibTorrentApi* libTorrentApi, QList<QSharedPointer<Repository>>& updateRepositories);
    void updateRepositoriesOffline(LibTorrentApi* libTorrentApi, QList<QSharedPointer<Repository>>& repositories);

protected:
    virtual bool writeJsonBytes(const QByteArray& bytes);
    [[nodiscard]] virtual QByteArray readJsonBytes() const;

private:
    static const QString JSON_RELATIVE_PATH;

    QVariantMap jsonMap_;
    SyncNetworkAccessManager nam_;

    bool updateJsonMap();
    [[nodiscard]] QString updateUrl(const QVariantMap& jsonMap) const;
    [[nodiscard]] QString updateUrl() const;
    QByteArray fetchJsonBytes(QString url);
    bool readJsonFile();
    [[nodiscard]] QSet<QString> getRemovablesOffline(const QList<QSharedPointer<Repository>>& updatedRepositories) const;
};

#endif // JSONREADER_H
