#include <QTimer>
#include "console.h"
#include "debug.h"

Console::Console(QObject* parent):
    QObject(parent)
{
    process_ = new QProcess();
}

Console::~Console()
{
    process_->kill();
    delete process_;
}

bool Console::runCmd(const QString& cmd)
{
    DBG << "Running command:" << cmd.toStdString().c_str();

    process_->start(cmd);

    while (process_->state() != QProcess::NotRunning)
    {
        process_->waitForReadyRead();
        DBG << process_->readAll().toStdString().c_str();
    }
    return process_->exitStatus() == 0;
}

void Console::terminate()
{
    if (process_->state() == QProcess::ProcessState::Running)
    {
        DBG << "Terminating process" << process_->program();
        process_->terminate();
    }
}
