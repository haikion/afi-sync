#include <QStringList>
#include <QCoreApplication>
#include <QFileInfo>
#include "sigcheckprocess.h"
#include "debug.h"

const QString SigCheckProcess::DS_CHECK_SIGNATURE_PATH = "bin/DSCheckSignatures.exe";

SigCheckProcess::SigCheckProcess(const QString& modPath, const QSet<QString>& filePaths):
   modPath_(modPath),
   checkStatus_(CheckStatus::NOT_STARTED)
{
   qRegisterMetaType<CheckStatus>();
   QString keyPath = findKeyFolder(filePaths);
   if (keyPath == QString())
   {
       DBG << "Warning: BIS Signature check failed. KeyPath not found in" << modPath;
       checkStatus_ = SigCheckProcess::CheckStatus::FAILURE;
       emit checkDone(checkStatus_);
       return;
   }
   setProgram(QCoreApplication::applicationDirPath() + "/" + DS_CHECK_SIGNATURE_PATH + " -deep \"" + modPath + "/addons\" \"" + keyPath + "\"");
   connect(this, SIGNAL(readyRead()), this, SLOT(printOutput()));
   connect(this, SIGNAL(finished(int)), this, SLOT(processDone()));
   connect(this, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(handleError(QProcess::ProcessError)));
}

SigCheckProcess::SigCheckProcess(SigCheckProcess::CheckStatus startStatus):
    checkStatus_(startStatus)
{
}

SigCheckProcess::CheckStatus SigCheckProcess::checkStatus() const
{
    return checkStatus_;
}

QString SigCheckProcess::findKeyFolder(const QSet<QString>& filePaths) {
    for (const QString& path : filePaths)
    {
        if (path.endsWith(".BIKEY", Qt::CaseInsensitive))
        {
            return QFileInfo(path).absolutePath();
        }
    }
    return QString();
}

void SigCheckProcess::start()
{
    checkStatus_ = CheckStatus::CHECKING;
    DBG << "Running cmd:" << program().toStdString().c_str();
    QProcess::start();
}

QString SigCheckProcess::modPath() const
{
   return modPath_;
}

void SigCheckProcess::handleError(QProcess::ProcessError error)
{
    DBG << "ERROR: failed to execute" << "error =" << error << " test " << program();
    checkStatus_ = CheckStatus::FAILURE;
    processDone();
}

void SigCheckProcess::processDone()
{
    if (checkStatus_ == CheckStatus::CHECKING)
    {
        DBG << "Successfully checked" << modPath();
        checkStatus_ = CheckStatus::SUCCESS;
    }

    emit checkDone(checkStatus_);
}

void SigCheckProcess::printOutput()
{
    QString output = readAll();
    if (output.contains("bisign is wrong", Qt::CaseInsensitive) ||
            output.contains("does not match the hash calculated from data", Qt::CaseInsensitive))
    {
        checkStatus_ = CheckStatus::FAILURE;
    }
    DBG << output.toStdString().c_str();
}
