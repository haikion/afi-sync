#ifndef DELETABLEDETECTOR_H
#define DELETABLEDETECTOR_H

#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QList>
#include "interfaces/irepository.h"
#include "deletable.h"

class DeletableDetector
{
public:
    DeletableDetector(const QString& modDownloadPath, const QList<IRepository*>& repositories);

    qint64 totalSize();
    QStringList deletableNames();

private:
    static QList<QFileInfo> deletableFileInfos(const QStringList& activeMods, const QString& modDownloadPath);

    QList<Deletable> deletables_;
};

#endif // DELETABLEDETECTOR_H
