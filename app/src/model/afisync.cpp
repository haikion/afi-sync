#include "afisync.h"

#include <QRegularExpression>

#include "afisynclogger.h"
#include "deletabledetector.h"
#include "modadapter.h"

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

const QSet<QString> AfiSync::activeModNames(const IRepository* repository)
{
    QSet<QString> retVal;
    for (const auto& mod : repository->modAdapters())
    {
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
    LOG << "Delete inactive mods (Space used: " + QString::number(deletableDetector.totalSize()/1000000000) + " GB) rmdir /s" + modList;
}

QStringList AfiSync::filterDeprecatedPatches(const QStringList& allFiles, const QStringList& urls) {
    QStringList retVal = allFiles;
    for (const QString& fileName : allFiles)
    {
        for (const QString& url : urls)
        {
            if (url.contains(fileName))
            {
                retVal.removeAll(fileName);
            }
        }
    }
    return retVal;
}

QStringList AfiSync::getDeltaUrlsForMod(const QString& modName, const QString& hash, const QStringList& deltaUrls)
{
    QStringList retVal;
    int idx = -1;
    bool ok = false;
    // Find the first applicable patch
    for (const auto& url : deltaUrls)
    {
        const QRegularExpression exp{".*" + modName + "\\.([0-9]+)\\." + hash + ".*"};
        auto match = exp.match(url);
        if (match.hasMatch())
        {
            idx = match.captured(1).toInt(&ok);
            if (!ok)
            {
                LOG_WARNING << "Could not parse delta patch index!" << url;
                return {};
            }
            retVal.append(url);
        }
    }
    // Find the subsequent patches
    QRegularExpression exp2{modName + "\\.([0-9]+)\\..*"};
    for (const auto& url : deltaUrls)
    {
        auto match = exp2.match(url);
        if (match.hasMatch())
        {
            auto idx2 = match.captured(1).toInt(&ok);
            if (!ok)
            {
                LOG_WARNING << "Could not parse subsequent delta patches!" << url;
                return {};
            }
            if (idx < idx2)
            {
                retVal.append(url);
            }
        }
    };
    retVal.sort();
    return retVal;
}
