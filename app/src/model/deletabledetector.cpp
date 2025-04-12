#include "deletabledetector.h"

#include <QDirIterator>
#include <QFileInfo>

#include "afisync.h"
#include "fileutils.h"

DeletableDetector::DeletableDetector(const QString& modDownloadPath, const QList<IRepository*>& repositories)
{
    QStringList modNames = AfiSync::activeModNames(repositories);
    modNames.sort(Qt::CaseInsensitive);
    QList<QFileInfo> files = deletableFileInfos(modNames, modDownloadPath);
    deletables_.reserve(files.size());
    for (const QFileInfo& file : files)
    {
        Deletable deletable(file.fileName(), FileUtils::dirSize(file.filePath()));
        deletables_.append(deletable);
    }
}

qint64 DeletableDetector::totalSize() const
{
    qint64 totalSize = 0;
    for (const Deletable& deletable : deletables_)
    {
        totalSize += deletable.size();
    }
    return totalSize;
}

QStringList DeletableDetector::deletableNames() const
{
    QStringList deletableNames;
    deletableNames.reserve(deletables_.size());
    for (const Deletable& deletable : deletables_)
    {
        deletableNames.append(deletable.name());
    }
    return deletableNames;
}

QList<QFileInfo> DeletableDetector::deletableFileInfos(const QStringList& activeMods, const QString& modDownloadPath)
{
    QList<QFileInfo> retVal;
    QDirIterator it(modDownloadPath, QDir::Dirs);
    while (it.hasNext())
    {
        const QFileInfo fileInfo(it.next());
        const QString fileName = fileInfo.fileName();
        if (fileName.startsWith('@') && !activeMods.contains(fileName)) {
            retVal.append(fileInfo);
        }
    }
    return retVal;
}
