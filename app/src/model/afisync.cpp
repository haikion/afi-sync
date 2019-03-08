#include "deletabledetector.h"
#include "interfaces/isyncitem.h"
#include "interfaces/imod.h"
#include "afisync.h"
#include "afisynclogger.h"

QStringList AfiSync::activeModNames(const QList<IRepository*>& repositories)
{
    QStringList retVal;
    for (const IRepository* repository : repositories)
    {
        for (const QString& modName : activeModNames(repository))
        {
            if (!retVal.contains(modName))
            {
                retVal.append(modName);
            }
        }
    }
    return retVal;
}

QSet<QString> AfiSync::activeModNames(const IRepository* repository)
{
    QSet<QString> retVal;
    for (IMod* mod : repository->uiMods())
    {
        //FIXME: Incorrect?
        if (mod->selected())
        {
            retVal.insert(mod->name());
        }
    }
    return retVal;
}

void AfiSync::printDeletables(const DeletableDetector& deletableDetector)
{
    QString modList;
    for (const QString& modName : deletableDetector.deletableNames())
    {
        modList += " " + modName;
    }
    LOG << "Delete inactive mods (Space used: " + QString::number(deletableDetector.totalSize()/1000000000) + " GB ) rmdir /s" + modList;
}
