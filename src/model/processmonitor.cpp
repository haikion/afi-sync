#include <QProcess>
#include <QCoreApplication>
#include "processmonitor.h"
#include "debug.h"

const QString ProcessMonitor::ARMA3_PROCESS = "arma3.exe";
const QString ProcessMonitor::ARMA3_PROCESS_BE = "arma3battleye.exe";
const QString ProcessMonitor::ARMA3_LAUNCHER = "arma3launcher.exe";
const QString ProcessMonitor::AFISYNC_PROCESS = "AFISync.exe";

ProcessMonitor::ProcessMonitor(QObject* parent): QObject(parent)
{
    DBG;
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
    params << "/NH" << "/FO" << "CSV" << "/FI" << QString("IMAGENAME eq %1").arg(process);
    if (pidFilter != 0)
        params << "/FI" << QString("PID ne %1").arg(QString::number(pidFilter));

    tasklist.start("tasklist", params);
    tasklist.waitForFinished();
    QString output = tasklist.readAllStandardOutput();
    return output.startsWith(QString("\"%1").arg(process));
}
