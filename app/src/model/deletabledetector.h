#ifndef DELETABLEDETECTOR_H
#define DELETABLEDETECTOR_H

#include <QFileInfo>
#include <QList>
#include <QString>
#include <QStringList>

#include "deletable.h"
#include "interfaces/irepository.h"

class DeletableDetector
{
public:
    DeletableDetector(const QString& modDownloadPath, const QList<IRepository*>& repositories);

    [[nodiscard]] qint64 totalSize() const;
    [[nodiscard]] QStringList deletableNames() const;

private:
    static QList<QFileInfo> deletableFileInfos(const QStringList& activeMods, const QString& modDownloadPath);

    QList<Deletable> deletables_;
};

#endif // DELETABLEDETECTOR_H
