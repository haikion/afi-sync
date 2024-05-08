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

std::ostream& operator<<(std::ostream& outStream, const QString& qString);
std::ostream& operator<<(std::ostream& outStream, const QList<int>& qVector);
std::ostream& operator<<(std::ostream& outStream, const QStringList& qList);
std::ostream& operator<<(std::ostream& outStream, const QSet<QString>& qSet);

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
    static const QString SZIP_EXECUTABLE;
    Szip szip_;

    void rotateLogs();
};

#endif // AFISYNCLOGGER_H
