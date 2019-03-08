#include "deletabledetector.h"
#include "fileutils.h"
#include "afisynclogger.h"
#include "interfaces/isyncitem.h"
#include "afisync.h"
#include <QDirIterator>
#include <QFileInfo>

DeletableDetector::DeletableDetector(const QString& modDownloadPath, const QList<IRepository*>& repositories)
{
    QStringList modNames = AfiSync::activeModNames(repositories);
    QList<QFileInfo> files = deletableFileInfos(modNames, modDownloadPath);
    for (const QFileInfo& file : files)
    {
        Deletable deletable(file.fileName(), FileUtils::dirSize(file.filePath()));
        deletables_.append(deletable);
    }
}

qint64 DeletableDetector::totalSize()
{
    qint64 totalSize = 0;
    for (const Deletable& deletable : deletables_)
    {
        totalSize += deletable.size();
    }
    return totalSize;
}

QStringList DeletableDetector::deletableNames()
{
    QStringList deletableNames;
    for (const Deletable& deletable : deletables_)
    {
        deletableNames.append(deletable.name());
    }
    return deletableNames;
}

/*
void DeletableDetector::printDeletables(const QList<IRepository*> repositories)
{
    printDeletables(deletables(activeMods(repositories)));
}
*/

QList<QFileInfo> DeletableDetector::deletableFileInfos(const QStringList& activeMods, const QString& modDownloadPath)
{
    QList<QFileInfo> retVal;
    QDirIterator it(modDownloadPath, QDir::Dirs);
    while (it.hasNext())
    {
        const QFileInfo fileInfo(it.next());
        const QString fileName = fileInfo.fileName();
        if (fileName.startsWith("@") && !activeMods.contains(fileName))
        {
            retVal.append(fileInfo);
        }
    }
    return retVal;
}
/*
void DeletableDetector::printDeletables(const QList<QFileInfo>& deletables)
{
    QString inactivesStr;
    qint64 spaceInactive = 0;

    for (const QFileInfo& fileInfo : deletables)
    {
        inactivesStr += " " + fileInfo.fileName();
        spaceInactive += FileUtils::dirSize(fileInfo.filePath());
    }
    LOG << "Delete inactive mods (Space used: " + QString::number(spaceInactive/1000000000) + " GB ) rmdir" + inactivesStr;
}
*/
