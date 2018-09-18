/*
 * Handles log rotation
 */
#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <boost/log/trivial.hpp>
#include <QSet>
#include <QString>
#include <QVector>
#include "runningtime.h"
#include "szip.h"
#include "qstreams.h"

#define LOG BOOST_LOG_TRIVIAL(info) << " " << Q_FUNC_INFO << " "
#define LOG_ERROR BOOST_LOG_TRIVIAL(error) << " " << Q_FUNC_INFO << " "
#define LOG_WARNING BOOST_LOG_TRIVIAL(warning) << " " << Q_FUNC_INFO << " "

class AfiSyncLogger
{
public:
    AfiSyncLogger() = default;

    void initFileLogging();

private:
    static const int MAX_LOGS_SIZE;
    static const QString SZIP_EXECUTABLE;
    Szip szip_; //Do not kill process when block ends

    QProcess* rotateLogs();
};

#endif // LOGMANAGER_H
