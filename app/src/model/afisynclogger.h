/*
 * Handles log rotation
 */
#ifndef AFISYNCLOGGER_H
#define AFISYNCLOGGER_H

#include <sstream>

#include <boost/log/trivial.hpp>

#include <QSet>
#include <QString>
#include <QVector>

#include "szip.h"

std::ostream& operator<<(std::ostream& outStream, const QString& qString);
std::ostream& operator<<(std::ostream& outStream, const QList<int>& qVector);
std::ostream& operator<<(std::ostream& outStream, const QStringList& qList);
std::ostream& operator<<(std::ostream& outStream, const QSet<QString>& qSet);

// Cleans MSVC function names
QString cleanFunc(auto fnc)
{
    QString clean{fnc};
    clean.remove("__cdecl");
    clean.replace("(void)", "()");
    clean.append(' ');
    return clean;
}

#ifdef Q_OS_WIN
#pragma warning(push, 0)
#endif

enum class LogLevel {
    Info,
    Warning,
    Error
};

class LogStream {
public:
    LogStream(const QString& funcInfo, LogLevel level)
        : funcInfo_{funcInfo},
        level_{level}
    {}
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    ~LogStream() {
        using std::string;

        string message = stream_.str();
        string functionInfo = funcInfo_.toStdString();
        if (message.length() <= 250)
        {
            message.append(250 - message.length(), ' ');
            message += functionInfo;
        }
        else
        {
            message += ' ' + functionInfo;
        }
        switch (level_)
        {
            case LogLevel::Info:
                BOOST_LOG_TRIVIAL(info) << message;
                break;
            case LogLevel::Warning:
                BOOST_LOG_TRIVIAL(warning) << message;
                break;
            case LogLevel::Error:
                BOOST_LOG_TRIVIAL(error) << message;
                break;
        }
    }

private:
    QString funcInfo_;
    LogLevel level_;
    std::ostringstream stream_;
};

#define LOG LogStream{cleanFunc(Q_FUNC_INFO), LogLevel::Info}
#define LOG_ERROR LogStream{cleanFunc(Q_FUNC_INFO), LogLevel::Error}
#define LOG_WARNING LogStream{cleanFunc(Q_FUNC_INFO), LogLevel::Warning}

#ifdef Q_OS_WIN
#pragma warning(pop)
#endif

class AfiSyncLogger
{
public:
    AfiSyncLogger() = default;
    virtual ~AfiSyncLogger();

    void initFileLogging();
    static void initConsoleLogging();

private:
    static const QString SZIP_EXECUTABLE;
    Szip szip_;

    void rotateLogs();
};

#endif // AFISYNCLOGGER_H
