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
#include "console.h"
#include "szip.h"
#include "qstreams.h"

#define LOG BOOST_LOG_TRIVIAL(info) << runningTimeMs() << Q_FUNC_INFO

class AfiSyncLogger
{
public:    
    static void initFileLogging();

private:
    static const int MAX_LOGS_SIZE;
    static const QString SZIP_EXECUTABLE;

    static bool rotateLogs();
};

#endif // LOGMANAGER_H
