/*
 * Simulates command line.
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
    ~Console() override;

    QProcess* runCmdAsync(const QString& cmd);

public slots:
    bool runCmd(const QString& cmd);
    void terminate();

private:
    QProcess* process_{nullptr};
};

#endif // CONSOLE_H
