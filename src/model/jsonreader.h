#ifndef JSONREADER_H
#define JSONREADER_H

#include <vector>
#include <QJsonDocument>
#include <QVariantMap>
#include <QString>
#include <QHash>
#include "mod.h"
#include "repository.h"

class JsonReader: public QObject
{
    Q_OBJECT

public:
    //Fills rootItem according to XML file
    static void fillEverything(RootItem* root);

signals:
    void modAppend(Mod* mod);

private:
    static const QString FILE_PATH;
    static const QString DOWNLOADED_PATH;

    static QJsonDocument openJsonFile(const QString& path);
    static bool updateJson(const QString& url);
};

#endif // JSONREADER_H
