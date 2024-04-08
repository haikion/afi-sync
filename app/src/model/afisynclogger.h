/*
 * Handles log rotation
 */
#ifndef AFISYNCLOGGER_H
#define AFISYNCLOGGER_H

#include <boost/log/trivial.hpp>
#include <QSet>
#include <QString>
#include <QVector>
#include "szip.h"
#include "qstreams.h"

#ifdef Q_OS_WIN
#pragma warning(push, 0)
#endif
#define LOG BOOST_LOG_TRIVIAL(info) << " " << Q_FUNC_INFO << " "
#define LOG_ERROR BOOST_LOG_TRIVIAL(error) << " " << Q_FUNC_INFO << " "
#define LOG_WARNING BOOST_LOG_TRIVIAL(warning) << " " << Q_FUNC_INFO << " "
#ifdef Q_OS_WIN
#pragma warning(pop)
#endif

class AfiSyncLogger
{
public:
    AfiSyncLogger() = default;

    void initFileLogging();

private:
    static const int MAX_LOGS_SIZE;
    static const QString SZIP_EXECUTABLE;
    Szip szip_; //Do not kill process when block ends

    bool rotateLogs();
};

#endif // AFISYNCLOGGER_H
