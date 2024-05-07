#include <QCoreApplication>
#include <QProcess>
#include <QStringLiteral>

#include "processmonitor.h"
#include "afisynclogger.h"

using namespace Qt::StringLiterals;

const QString ProcessMonitor::ARMA3_PROCESS = u"arma3.exe"_s;
const QString ProcessMonitor::ARMA3_PROCESS_BE = u"arma3battleye.exe"_s;
const QString ProcessMonitor::ARMA3_LAUNCHER = u"arma3launcher.exe"_s;
const QString ProcessMonitor::AFISYNC_PROCESS = u"AFISync.exe"_s;

ProcessMonitor::ProcessMonitor(QObject* parent): QObject(parent)
{
    LOG;
}

bool ProcessMonitor::arma3Running()
{
    return isRunning(ARMA3_PROCESS) || isRunning(ARMA3_PROCESS_BE) || isRunning(ARMA3_LAUNCHER);
}

bool ProcessMonitor::afiSyncRunning()
{
    return isRunning(AFISYNC_PROCESS, QCoreApplication::applicationPid());
}

bool ProcessMonitor::isRunning(const QString& process, const qint64 pidFilter)
{
    #ifndef Q_OS_WIN
        return false;
    #endif
    QProcess tasklist;
    QStringList params;
    params << u"/NH"_s << u"/FO"_s << u"CSV"_s
           << u"/FI"_s << u"IMAGENAME eq %1"_s.arg(process);
    if (pidFilter != 0)
    {
        params << u"/FI"_s
               << u"PID ne %1"_s.arg(QString::number(pidFilter));
    }

    tasklist.start(u"tasklist"_s, params);
    tasklist.waitForFinished();
    QString output = tasklist.readAllStandardOutput();
    return output.startsWith(QStringLiteral("\"%1").arg(process));
}
