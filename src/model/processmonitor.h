#ifndef PROCESSMONITOR_H
#define PROCESSMONITOR_H
#include <QObject>
#include <QString>

class ProcessMonitor: public QObject
{
    Q_OBJECT

public:
    explicit ProcessMonitor(QObject* parent = 0);

public slots:
    static bool arma3Running();
    static bool afiSyncRunning();

private:
    static const QString ARMA3_PROCESS;
    static const QString ARMA3_PROCESS_BE;
    static const QString ARMA3_LAUNCHER;
    static const QString AFISYNC_PROCESS;

    bool static isRunning(const QString& process, const qint64 pidFilter = 0);
};

#endif // PROCESSMONITOR_H
