#include <QEventLoop>
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
    DBG << "Running command:" << cmd;

    process_->start(cmd);

    bool rVal = process_->waitForFinished();
    DBG << process_->readAll().toStdString().c_str();
    return rVal;
}
