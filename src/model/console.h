/*
 * Simulates command line.
 *
 * TODO: Move to hoxlib
 */

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QObject>
#include <QString>
#include <QProcess>

class Console: public QObject
{
public:
    Console(QObject* parent = nullptr);
    ~Console();

public slots:
    bool runCmd(const QString& cmd);
    void terminate();

private:
    QProcess* process_;

    bool waitFinished(QProcess* process) const;
};

#endif // CONSOLE_H
