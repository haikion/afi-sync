#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/attr.hpp>
#include <QFile>
#include <QDateTime>
#include <QDirIterator>
#include <QStringList>
#include "global.h"
#include "afisynclogger.h"
#include "fileutils.h"

using namespace boost::log;

#ifdef Q_OS_LINUX
    const QString AfiSyncLogger::SZIP_EXECUTABLE = "7za";
#else
    const QString AfiSyncLogger::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

const int MAX_LOG_FILES = 3;

void AfiSyncLogger::initFileLogging()
{
    rotateLogs();

    add_file_log
    (
        keywords::file_name = Constants::LOG_FILE.toStdString(),
        // This makes the sink to write log records that look like this:
        // 1: [normal] A normal severity message
        // 2: [error] An error severity message
        keywords::format =
        (
            expressions::stream << " [" << boost::log::trivial::severity
                << "] " << expressions::smessage
        )
    );
}

bool AfiSyncLogger::rotateLogs()
{
    const QFile logFile(Constants::LOG_FILE);
    if (!logFile.exists())
        return false;

    QStringList patchArchives =
            QDir(".").entryList(QDir::Files).filter(QRegExp(Constants::LOG_FILE + ".*7z" ));

    patchArchives.sort();
    for (int i = 0; i <= (patchArchives.size() - MAX_LOG_FILES); ++i)
    {
        const QString deleteThis = patchArchives.at(i);
        LOG << "Deleting old log file " << deleteThis;
        FileUtils::safeRemove(deleteThis);
    }
    const QString filePrefix = QString(Constants::LOG_FILE) + "_" + QDateTime::currentDateTime().toString("yyyy.MM.dd.hh.mm.ss");
    const QString logName =  filePrefix + ".log";
    FileUtils::move(Constants::LOG_FILE, logName);
    Szip szip;
    return szip.compressAsync(logName, filePrefix + ".7z");
}
