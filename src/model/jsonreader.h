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

class JsonReader: public QObject
{
    Q_OBJECT

public:
    //Fills rootItem according to XML file
    void fillEverything(RootItem* root);
    //Enables better testability.
    void fillEverything(RootItem *root, const QString& jsonFilePath);
    bool updateAvaible();

signals:
    void modAppend(Mod* mod);

private:
    static const QString FILE_PATH;
    static const QString DOWNLOADED_PATH;
    static const QString SEPARATOR;

    QVariantMap jsonMap_;
    SyncNetworkAccessManager nam_;

    QJsonDocument readJsonFile(const QString& path) const;
    QVariantMap updateJson(const QString& url);
    QString updateUrl(const QVariantMap& jsonMap) const;
    QSet<QString> addedMods(const RootItem* root) const;
    QHash<QString, Repository*> addedRepos(const RootItem* root) const;
};

#endif // JSONREADER_H
