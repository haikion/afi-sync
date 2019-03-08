#include "interfaces/isyncitem.h"
#include "afisync.h"

QStringList AfiSync::activeModNames(const QList<IRepository*>& repositories)
{
    QStringList retVal;
    for (const IRepository* repository : repositories)
    {
        for (const QString& modName : activeModNames(repository))
        {
            retVal.append(modName);
        }
    }
    return retVal;
}

QSet<QString> AfiSync::activeModNames(const IRepository* repository)
{
    QSet<QString> retVal;
    for (ISyncItem* mod : repository->uiMods())
    {
        //FIXME: Incorrect?
        if (mod->ticked())
        {
            retVal.insert(mod->name());
        }
    }
    return retVal;
}
