#ifndef PROCESSMONITOR_H
#define PROCESSMONITOR_H

#include <QObject>
#include <QString>

class ProcessMonitor: public QObject
{
    Q_OBJECT

public:
    explicit ProcessMonitor(QObject* parent = nullptr);

public:
    [[nodiscard]] static bool arma3Running();
    [[nodiscard]] static bool afiSyncRunning();

private:
    static const QString ARMA3_PROCESS;
    static const QString ARMA3_PROCESS_BE;
    static const QString ARMA3_LAUNCHER;
    static const QString AFISYNC_PROCESS;

    [[nodiscard]] bool static isRunning(const QString& process, qint64 pidFilter = 0);
};

#endif // PROCESSMONITOR_H
