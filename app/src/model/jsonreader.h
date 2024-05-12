#ifndef JSONREADER_H
#define JSONREADER_H

#include <QByteArray>
#include <QHash>
#include <QJsonDocument>
#include <QList>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QVariantMap>

#include "apis/isync.h"
#include "syncnetworkaccessmanager.h"

class LibTorrentApi;
class Repository;
using RepositoryList = QList<QSharedPointer<Repository>>;

class JsonReader
{
public:
    JsonReader() = default;
    virtual ~JsonReader() = default;

    void readJson();
    bool updateAvailable();
    [[nodiscard]] QList<Repository*> repositories(ISync* sync);
    [[nodiscard]] QStringList deltaUrls();
    [[nodiscard]] QSet<QString> getRemovables(const RepositoryList& updateRepositories);
    void updateRepositories(LibTorrentApi* libTorrentApi, RepositoryList& updateRepositories);
    void updateRepositoriesOffline(LibTorrentApi* libTorrentApi, RepositoryList& repositories);

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
    [[nodiscard]] QSet<QString> getRemovablesOffline(const RepositoryList& updatedRepositories) const;
};

#endif // JSONREADER_H
