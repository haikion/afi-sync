#include "afisynclogger.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/attr.hpp>
#include <boost/log/expressions/attr_fwd.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <QDateTime>
#include <QDirIterator>
#include <QFile>
#include <QStringList>
#include <QStringLiteral>

#include "global.h"
#include "fileutils.h"

using namespace Qt::StringLiterals;
using namespace boost::log;
using std::ostream;

ostream& operator<<(std::ostream& outStream, const QString& qString)
{
    outStream << qString.toStdString();
    return outStream;
}

ostream& operator<<(std::ostream& outStream, const QList<int>& qVector)
{
    if (qVector.isEmpty())
    {
        outStream << " QVector[]";
        return outStream;
    }
    outStream << " QVector[" << qVector.at(0);
    for (int i = 1; i < qVector.size(); ++i)
    {
        outStream << ", " << qVector.at(i);
    }
    outStream << "]";
    return outStream;
}

ostream& operator<<(std::ostream& outStream, const QStringList& qList)
{
    using std::string;

    if (qList.isEmpty())
    {
        outStream << " []";
        // outStream << " []";
        return outStream;
    }
    QString str = u" ["_s + qList.at(0);
    for (int i = 1; i < qList.size(); ++i)
    {
        str += u", "_s + qList.at(i);
    }
    str += u"]"_s;
    outStream << str;
    return outStream;
}

ostream& operator<<(std::ostream& outStream, const QSet<QString>& qSet)
{
    outStream << qSet.values();
    return outStream;
}

#ifdef Q_OS_LINUX
const QString AfiSyncLogger::SZIP_EXECUTABLE = u"7za"_s;
#else
    const QString AfiSyncLogger::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

namespace
{
    const int MAX_LOG_FILES = 3;
}

AfiSyncLogger::~AfiSyncLogger()
{
    boost::log::core::get()->flush();
}

void AfiSyncLogger::initFileLogging()
{
    rotateLogs();
    boost::log::add_common_attributes();

    add_file_log
    (
        keywords::file_name = Constants::LOG_FILE.toStdString(),
        // This makes the sink to write log records that look like this:
        // 1: [normal] A normal severity message
        // 2: [error] An error severity message
        keywords::format =
        (
            expressions::stream
                <<  expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
                << " [" << trivial::severity << "] " << expressions::smessage
        ),
        keywords::auto_flush = true
    );
}

void AfiSyncLogger::initConsoleLogging()
{
    boost::log::add_common_attributes();

    auto console_sink = boost::log::add_console_log
    (
        std::cout,
        keywords::format =
        (
            expressions::stream
                << expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
                << " [" << trivial::severity << "] " << expressions::smessage
        ),
        keywords::auto_flush = true
    );
}

void AfiSyncLogger::rotateLogs()
{
    const QFile logFile(Constants::LOG_FILE);
    if (!logFile.exists())
    {
        return;
    }

    static const QRegularExpression regExp{Constants::LOG_FILE + ".*7z"};
    QStringList patchArchives = QDir(u"."_s).entryList(QDir::Files).filter(regExp);

    patchArchives.sort();
    for (int i = 0; i <= (patchArchives.size() - MAX_LOG_FILES); ++i)
    {
        const QString deleteThis = patchArchives.at(i);
        FileUtils::safeRemove(deleteThis);
    }
    const QString filePrefix = QString(Constants::LOG_FILE) + "_"
                               + QDateTime::currentDateTime().toString(
                                   u"yyyy.MM.dd.hh.mm.ss"_s);
    const QString logName =  filePrefix + ".log";
    FileUtils::move(Constants::LOG_FILE, logName);
    szip_.compressAsync(logName, filePrefix + ".7z");
}
