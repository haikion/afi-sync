#include <QFile>
#include <QDateTime>
#include <QDirIterator>
#include <QStringList>
#include "global.h"
#include "logmanager.h"
#include "debug.h"
#include "fileutils.h"

#ifdef Q_OS_LINUX
    const QString LogManager::SZIP_EXECUTABLE = "7za";
#else
    const QString LogManager::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

const int MAX_LOG_FILES = 3;

bool LogManager::rotateLogs()
{
    QFile logFile(Constants::LOG_FILE);
    if (!logFile.exists())
        return false;

    QStringList patchArchives =
            QDir(".").entryList(QDir::Files).filter(QRegExp(Constants::LOG_FILE + ".*7z" ));

    patchArchives.sort();
    for (int i = 0; i <= (patchArchives.size() - MAX_LOG_FILES); ++i)
    {
        QString deleteThis = patchArchives.at(i);
        DBG << "Deleting old log file" << deleteThis;
        FileUtils::safeRemove(deleteThis);
    }
    return szip_.compress(Constants::LOG_FILE, QString(Constants::LOG_FILE).append("_" +
             QDateTime::currentDateTime().toString("yyyy.MM.dd.hh.mm.ss") + ".7z"));
}
