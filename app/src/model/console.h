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
    Q_OBJECT

public:
    Console(QObject* parent = nullptr);
    ~Console();

    QProcess* runCmdAsync(const QString& cmd);
    bool runCmd(const QString& cmd);

public slots:
    void terminate();

private:
    QProcess process_;
};

#endif // CONSOLE_H
