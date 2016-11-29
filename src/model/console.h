#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QProcess>

class Console: public QObject
{
public:
    Console(QObject* parent = nullptr);
    ~Console();

public slots:
    bool runCmd(const QString& cmd);

private:
    QProcess* process_;
    QThread thread_;
    bool waitFinished(QProcess* process) const;
};

#endif // CONSOLE_H
