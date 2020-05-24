#include "afisynclogger.h"
#include "console.h"

Console::Console(QObject* parent):
    QObject(parent)
{
    process_ = new QProcess();
    connect(process_, &QProcess::channelReadyRead, this, [=] (int) {
        const QByteArray output = process_->readAllStandardOutput();
        if (!output.isEmpty()) {
            LOG << output.toStdString().c_str();
        }
        const QByteArray errorOutput = process_->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            LOG << errorOutput.toStdString().c_str();
        }
    });
}

Console::~Console()
{
    process_->kill();
    delete process_;
}

bool Console::runCmd(const QString& cmd) const
{
    LOG << "Running command: " << cmd.toStdString().c_str();

    process_->start(cmd);
    process_->waitForFinished(-1);

    return process_->exitCode() == 0;
}

QProcess* Console::runCmdAsync(const QString& cmd)
{
    LOG << "Running command: " << cmd.toStdString().c_str();

    process_->start(cmd);
    return process_;
}

void Console::terminate()
{
    if (process_->state() == QProcess::ProcessState::Running)
    {
        LOG << "Terminating process " << process_->program();
        process_->terminate();
    }
}
