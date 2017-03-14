#include <QTimer>
#include "console.h"
#include "debug.h"

Console::Console(QObject* parent):
    QObject(parent)
{
    process_ = new QProcess();
    connect(process_, SIGNAL(readyRead()), this, SLOT(printOutput()));
}

Console::~Console()
{
    process_->kill();
    delete process_;
}

bool Console::runCmd(const QString& cmd) const
{
    DBG << "Running command:" << cmd.toStdString().c_str();

    process_->start(cmd);
    process_->waitForFinished();

    return process_->exitCode() == 0;
}

QProcess* Console::runCmdAsync(const QString& cmd)
{
    DBG << "Running command:" << cmd.toStdString().c_str();

    process_->start(cmd);
    return process_;
}

void Console::printOutput()
{
    DBG << process_->readAll().toStdString().c_str();
}

void Console::terminate()
{
    if (process_->state() == QProcess::ProcessState::Running)
    {
        DBG << "Terminating process" << process_->program();
        process_->terminate();
    }
}
