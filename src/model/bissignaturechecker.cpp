#include <QString>
#include "bissignaturechecker.h"
#include "fileutils.h"
#include "console.h"
#include "debug.h"

BisSignatureChecker::BisSignatureChecker(QObject* parent):
    QObject(parent)
{
    DBG;
}

QSharedPointer<SigCheckProcess> BisSignatureChecker::check(const QString& modPath, const QSet<QString>& filePaths)
{
    DBG << "Cheking" << modPath;
    if (!FileUtils::filesExistCi(filePaths))
    {
        DBG << "Warning: BIS Signature check failed. Missing files.";
        //Return failed task
        return QSharedPointer<SigCheckProcess>(new SigCheckProcess(SigCheckProcess::CheckStatus::FAILURE));
    }

    QSharedPointer<SigCheckProcess> process = createProcess(modPath, filePaths);
    if (process->checkStatus() == SigCheckProcess::CheckStatus::FAILURE)
    {
        return process;
    }

    executionQueue_.enqueue(process);
    if (executionQueue_.size() == 1)
    {
        process->start();
    }

    return QSharedPointer<SigCheckProcess>(process);
}

bool BisSignatureChecker::isQueued(const QString& modPath) const
{
    //First one is being checked
    for (int i = 1; i < executionQueue_.size(); ++i)
    {
        if (executionQueue_.at(i)->modPath() == modPath)
        {
            return true;
        }
    }

    return false;
}

bool BisSignatureChecker::isChecking(const QString& modPath) const
{
    if (executionQueue_.isEmpty())
    {
        return false;
    }
    return executionQueue_.head()->modPath() == modPath;
}

QSharedPointer<SigCheckProcess> BisSignatureChecker::createProcess(const QString& modPath, const QSet<QString>& filePaths) const
{
    QSharedPointer<SigCheckProcess> process = QSharedPointer<SigCheckProcess>(new SigCheckProcess(modPath, filePaths), doDeleteLater);
    connect(process.data(), SIGNAL(checkDone(SigCheckProcess::CheckStatus)), this, SLOT(processQueue()));
    return process;
}

void BisSignatureChecker::processQueue()
{
    executionQueue_.dequeue(); //Remove completed
    if (executionQueue_.isEmpty())
    {
        DBG << "Emiting done";
        emit checkDone();
        return;
    }
    QSharedPointer<SigCheckProcess> process = executionQueue_.head();
    process->start();
}
