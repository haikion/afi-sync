#include "deletabledetector.h"
#include "settingsmodel.h"
#include "mod.h"
#include "fileutils.h"
#include "afisynclogger.h"
#include <QDirIterator>

QStringList DeletableDetector::activeMods(const QList<Repository*> repositories)
{
    QStringList retVal;
    for (const Repository* repository : repositories)
    {
        for (const QString& modName : activeMods(repository))
        {
            retVal.append(modName);
        }
    }
    return retVal;
}

QSet<QString> DeletableDetector::activeMods(const Repository* repository)
{
    QSet<QString> retVal;
    for (Mod* mod : repository->mods())
    {
        //FIXME: Incorrect?
        if (mod->ticked())
        {
            retVal.insert(mod->name());
        }
    }
    return retVal;
}

void DeletableDetector::printDeletables(const QList<Repository*> repositories)
{
    printDeletables(deletables(activeMods(repositories)));
}

QList<QFileInfo> DeletableDetector::deletables(const QStringList& activeMods)
{
    QList<QFileInfo> retVal;
    QDirIterator it(SettingsModel::modDownloadPath(), QDir::Dirs);
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
