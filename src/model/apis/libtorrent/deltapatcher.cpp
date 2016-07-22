#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>
#include <QStringList>
#include <QDirIterator>
#include <QRegExp>
#include <QDebug>
#include "../../fileutils.h"
#include "../../debug.h"
#include "ahasher.h"
#include "deltapatcher.h"

#ifdef Q_OS_LINUX
    const QString DeltaPatcher::XDELTA_EXECUTABLE = "xdelta3";
    const QString DeltaPatcher::SZIP_EXECUTABLE = "7za";
#else
    const QString DeltaPatcher::XDELTA_EXECUTABLE = "bin/xdelta3.exe";
    const QString DeltaPatcher::SZIP_EXECUTABLE = "bin/7za.exe";
#endif

//20 minutes. 7za compression may take a while
const int DeltaPatcher::TIMEOUT = 20*60000;
const QString DeltaPatcher::DELTA_POSTFIX = "_patch";
const QString DeltaPatcher::DELTA_EXTENSION = ".vcdiff";
const QString DeltaPatcher::SEPARATOR = ".";
const QString DeltaPatcher::PATCH_DIR = "delta_patch";


DeltaPatcher::DeltaPatcher(const QString& patchesPath)
{
    thread_ = new QThread(this);
    moveToThread(thread_);
    thread_->start();
    QMetaObject::invokeMethod(this, "threadConstructor", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, patchesPath));
}


void DeltaPatcher::threadConstructor(const QString& patchesPath)
{
    patchesFi_ = new QFileInfo(patchesPath);
    process_ = new QProcess(this);
}

DeltaPatcher::~DeltaPatcher()
{
    process_->kill();
    //ToDo: Cleanup
    thread_->quit();
    thread_->wait(5000);
    thread_->terminate();
    thread_->wait(1000);
    delete thread_;
    delete patchesFi_;
}

void DeltaPatcher::patch(const QString& modPath)
{
    QMetaObject::invokeMethod(this, "patchDirSync", Qt::QueuedConnection,
                              Q_ARG(QString, modPath));
}

void DeltaPatcher::patchDirSync(const QString& modPath)
{
    static const QString ERROR_NO_PATCHES = "ERROR: no patches found for";

    QDir patchesDir = QDir(patchesFi_->absoluteFilePath());
    const QStringList allPatches = patchesDir.entryList(QDir::Files);
    DBG << allPatches << patchesDir.absolutePath()
        << patchesFi_->absoluteFilePath() << modPath;
    QString modName = QFileInfo(modPath).fileName();
    QStringList patches = filterPatches(modPath, allPatches);
    if (patches.size() == 0)
    {
        DBG << ERROR_NO_PATCHES << modName << "from"
            << allPatches << "hash =" << AHasher::hash(modPath);
        emit patched(modPath, false);
        return;
    }
    DBG << patches;
    for (const QString& patchPath : patches)
    {
        if (!patch(patchPath, modPath))
        {
            emit patched(modPath, false);
            return;
        }
    }
    emit patched(modPath, true);
}

QStringList DeltaPatcher::filterPatches(const QString& modPath, const QStringList& allPatches)
{
    static const QString ERROR_NO_PATCHES = "ERROR: no patches found for";

    DBG << modPath << allPatches;
    QString modName = QFileInfo(modPath).fileName();
    int ltstVersion = latestVersion(modName, allPatches);
    if (ltstVersion == -1)
    {
        DBG << ERROR_NO_PATCHES << modName << "from" << allPatches;
        return QStringList();
    }
    QStringList patches;
    DBG << "modpath =" << modPath;
    QString hash = AHasher::hash(modPath);
    QRegExp regEx(modName + ".*" + hash + "\\" + SEPARATOR + "7z");
    QStringList matches = allPatches.filter(regEx);
    if (matches.size() != 1)
    {
        DBG << "ERROR: Incorrect number (" << matches.size()
                 << ") of suitable patches. Aborting...1";
        return QStringList();
    }
    QString patchName = matches.at(0);
    //First version.
    int version = patchName.split(SEPARATOR).at(1).toInt();
    patches.append(patchName);
    while (version != ltstVersion)
    {
        ++version;
        QRegExp regExp(modName + "\\." + QString::number(version) + ".*7z");
        QStringList matches = allPatches.filter(regExp);
        if (matches.size() != 1)
        {
            DBG << "ERROR: Incorrect number (" << matches.size()
                     << ") of suitable patches. Aborting...2";
            return QStringList();
        }
        patches.append(matches.at(0));
    }
    DBG << patches;
    return patches;
}

//Synchronous patch function
bool DeltaPatcher::patch(const QString& patch, const QString& modPath)
{
    QString patchPath = patchesFi_->absoluteFilePath() + "/" + patch;
    if (!extract(patchPath))
        return false;
    QString extractedPath = QFileInfo(patchPath).absolutePath() + "/" + PATCH_DIR;
    return patchExtracted(extractedPath, modPath);
}

//ToDo: Figure out what to do with paths.
bool DeltaPatcher::runCmd(const QString& cmd)
{
    DBG << "Running command:" << cmd;
    //return true;
    //if (QFileInfo(workingDir).exists())
    //{ TODO: Remove
    //    process.setWorkingDirectory(workingDir);
    //}
    process_->start(cmd);
    if (!waitFinished(process_))
        return false;

    return true;
}



bool DeltaPatcher::waitFinished(QProcess* process) const
{
    if (process->state() == QProcess::ProcessState::NotRunning)
    {
        DBG << "ERROR: Process is not running.";
        return false;
    }

    QEventLoop loop;
    QTimer::singleShot(TIMEOUT, &loop, SLOT(quit()));
    connect(process, SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();
    DBG << process->readAll().toStdString().c_str();

    if (process->exitCode() != 0)
    {
        DBG << "ERROR: Execution failed. Exit code =" << process->exitCode();
        return false;
    }
    return true;
}

//ToDo: Use static dirs
void DeltaPatcher::cleanUp(QDir& deltaDir, QDir& tmpDir)
{
    DBG << "Deleting dir" << deltaDir.absolutePath();
    deltaDir.removeRecursively(); //TODO: Uncomment
    DBG << "Deleting dir" << tmpDir.absolutePath();
    tmpDir.removeRecursively(); //TODO: Uncomment
}

bool DeltaPatcher::patchExtracted(const QString& extractedPath, const QString& targetPath)
{
    QDir deltaDir = QDir(extractedPath);
    QDir tmpDir = QDir(extractedPath + "_tmp");

    if (!createDir(tmpDir))
    {
        cleanUp(deltaDir, tmpDir);
        return false;
    }

    DBG << "extractedPath" << extractedPath;
    QDirIterator it(extractedPath, QDir::Files, QDirIterator::Subdirectories);
    bool pat = false; //True if something was patched
    if (!it.hasNext())
    {
        DBG << "ERROR: Nothing to patch in" << extractedPath;
        cleanUp(deltaDir, tmpDir);
        return false;
    }
    while (it.hasNext())
    {
        QFileInfo patchFile = it.next();
        QString diffPath = patchFile.absoluteFilePath();

        QString relPath = diffPath;
        relPath = relPath.remove(extractedPath).remove(DELTA_EXTENSION);
        QString patchedPath = tmpDir.absolutePath() + relPath;

        QDir().mkpath(QFileInfo(patchedPath).absolutePath());
        if (!diffPath.endsWith(DELTA_EXTENSION))
        {
            DBG << "Copying" << diffPath << "to" << patchedPath;
            QFile::copy(diffPath, patchedPath);
            continue;
        }
        QString targetFilePath = targetPath + relPath;

        bool rVal = runCmd(XDELTA_EXECUTABLE + " -d -s " + " \""
                           + QDir::toNativeSeparators(targetFilePath) + "\" \""
                           + QDir::toNativeSeparators(diffPath) + "\" \""
                           + QDir::toNativeSeparators(patchedPath)) + "\"";

        if (!rVal)
        {
            DBG << "Warning: Delta patching failed.";
            cleanUp(deltaDir, tmpDir);
            return false;
        }
        pat = true;
    }
    FileUtils::copy(tmpDir.absolutePath(), targetPath);
    cleanUp(deltaDir, tmpDir);
    DBG << "Delta patching succesfull!";
    return pat;
}

bool DeltaPatcher::delta(const QString& oldDir, QString laterPath)
{

    QString patchesPath = patchesFi_->absoluteFilePath();
    QDir patchesDir(patchesPath);
    QString deltaPath = patchesPath + "/" + PATCH_DIR;
    QDir deltaDir(deltaPath);
    QString oldHash = AHasher::hash(oldDir);
    QString modName = QFileInfo(oldDir).fileName();
    QString patchName = modName + SEPARATOR
            + QString::number(latestVersion(modName) + 1) + SEPARATOR + oldHash + ".7z";
    QString patchPath = patchesFi_->absoluteFilePath() + "/" + patchName;

    QFileInfo laterFi = QFileInfo(laterPath);
    if (!laterFi.exists())
    {
        DBG << "ERROR: Path" << laterPath << "does not exist.";
        return false;
    }
    laterPath = laterFi.absoluteFilePath();

    QStringList conflictingFiles =
            patchesDir.entryList().filter(QRegExp(modName + "\\..*\\." + oldHash + "\\.7z" ));
    if (conflictingFiles.size() > 0)
    {
        DBG << "ERROR: patch" << patchName << "collides with files:" << conflictingFiles;
        return false;
    }

    if (!patchesFi_->isWritable())
    {
        DBG << "ERROR: Directory" << patchesFi_->absoluteFilePath() << "is not writable.";
        return false;
    }

    if (deltaDir.exists())
    {
        DBG << "ERROR: Directory" << deltaPath << "already exists. Deleting...";
        deltaDir.removeRecursively();
    }

    QDir().mkpath(deltaPath);
    QDirIterator it(laterPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo newFile = it.next();

        QString relPath = newFile.absoluteFilePath().remove(laterPath);
        DBG << relPath << laterPath;
        QFileInfo oldFile = QFileInfo(oldDir + "/" + relPath);
        QString outputPath = deltaPath + relPath;
        QString parentPath = QFileInfo(outputPath).absolutePath();
        QString newPath = newFile.absoluteFilePath();

        QDir().mkpath(parentPath);
        if (!oldFile.exists())
        {
            DBG << "Copying" << newPath << "to" << outputPath;
            QFile::copy(newPath, outputPath);
            continue;
        }

        outputPath = outputPath + DELTA_EXTENSION;
        QString oldPath = QDir::toNativeSeparators(oldFile.absoluteFilePath());
        QString laterPath = QDir::toNativeSeparators(newPath);
        //Creates uncompressed delta patch file
        QMetaObject::invokeMethod(this, "runCmd", Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, XDELTA_EXECUTABLE + " -e -S none -s \""
                                        + oldPath + "\" \"" + laterPath + "\" \"" + outputPath + "\""));
    }

    QMetaObject::invokeMethod(this, "compress", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, patchPath), Q_ARG(QString, deltaPath));

    DBG << "Deleting directory" << deltaPath;
    deltaDir.removeRecursively(); //ToDo: uncomment

    return true;
}
/*
//Merges every vcdiff and appends file additions from both patches.
//files that not longer belong to the mod are removed.
//FIXME: Same names
void DeltaPatcher::merge(const QString& oldPatchZip,
                         const QString& newPatchPath,
                         const QString& newModDir) const
{
    static const QString MERGE_POSTFIX = "_merge";

    QFileInfo zipFile(oldPatchZip);
    //File name format: @modName.from.to.mergeCount.7z
    QStringList lst = zipFile.fileName().split(SEPARATOR);
    QString modName = lst.at(0);
    QString from = lst.at(1);
    QString to = QFileInfo(newPatchPath).fileName().split(SEPARATOR).at(2);
    int mergeCount = lst.at(3).toInt();
    QString extractedPath = zipFile.dir().absolutePath() + "/" + modName + DELTA_POSTFIX;
    QString mergedPath = extractedPath + MERGE_POSTFIX;
    QSet<QString> normalFiles;

    if (from == to)
    {
        DBG << "ERROR: File hashes are identical. Aborting...";
        return;
    }
    if (mergeCount > MAX_MERGES)
    {
        DBG << "Merge count exceedes the maximum of" << MAX_MERGES
                 << "aborting...";
        return;
    }

    extract(oldPatchZip);

    //Iterate through old patch files.
    //merge vcdiffs and append files.
    QDirIterator it(extractedPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo oldFile = it.next();
        QString oldFilePath = oldFile.absolutePath();
        QString relPath = oldFilePath.remove(extractedPath);

        QString mergedFilePath = mergedPath + relPath;
        QFileInfo mergedFileFile = QFileInfo(mergedFilePath);

        if (oldFile.fileName().endsWith(DELTA_EXTENSION))
        {
            QString newPatchPath = newPatchPath + relPath;
            QFileInfo newPatchFile(newPatchPath);

            if (!newPatchFile.exists())
            {
                DBG << "File" << oldFile.absolutePath()
                         << "not found in new patch. Ignoring...";
                continue;
            }

            QDir().mkpath(mergedFileFile.dir().absolutePath()); //Create parent dir

            runCmd(XDELTA_EXECUTABLE + " merge -m "
                   + QDir::toNativeSeparators(oldFilePath) + " "
                   + QDir::toNativeSeparators(newPatchPath) + " "
                   + QDir::toNativeSeparators(mergedFilePath));

            continue;
        }

        //Non vcdiff file. Append...
        QString modFilePath = newModDir + relPath;
        normalFiles.insert(modFilePath);
    }

    //Process non vcdiff files in later patch.
    QDirIterator it2(newPatchPath, QDir::Files, QDirIterator::Subdirectories);
    while (it2.hasNext())
    {
        QFileInfo newFile = it2.next();

        if (newFile.fileName().endsWith(DELTA_EXTENSION))
            continue;

        QString relPath = newFile.absolutePath().remove(newPatchPath);
        QString modFilePath = newModDir + relPath;
        normalFiles.insert(modFilePath);
    }

    //Filter out non vcdiff files that are no longer included in the mod.
    //Copy remaining into merged folder
    for (QString modFilePath : normalFiles)
    {
        QFileInfo modFile(modFilePath);

        if (!modFile.exists())
        {
            DBG << "file" << modFilePath << "does not exist in mod in anymore and"
                     << "will not be included in the patch.";
            continue;
        }

        QString relPath = modFilePath.remove(newModDir);
        QString mergedPath = mergedPath + relPath;
        QFileInfo mergedFileFile(mergedPath);

        QDir().mkpath(mergedFileFile.dir().absolutePath()); //Create parent dir
        QFile::copy(modFilePath, mergedPath);
    }
    DBG << "Removing directory" << extractedPath;
    //QDir(extractedPath).removeRecursively(); Todo: Uncomment
    DBG << "Renaming" << mergedPath  << "to" << extractedPath;
    //QDir::rename(mergedPath, extractedPath); Todo: Uncomment
    compress(extractedPath, modName + SEPARATOR + from
             + SEPARATOR + to + SEPARATOR + QString::number(mergeCount) + ".7z");

}
*/

bool DeltaPatcher::extract(const QString& zipPath)
{
    DBG << "Extracting" << zipPath;
    QFileInfo fi = QFileInfo(zipPath);
    return runCmd(SZIP_EXECUTABLE + " -y x \""
           + QDir::toNativeSeparators(zipPath)
           + "\" -o\"" + QDir::toNativeSeparators(fi.absolutePath()) + "\"");
}

void DeltaPatcher::compress(const QString& dir, const QString& archivePath)
{
    //Create 7zip archive. r = recurse, y = assume yes to everything
    //-t7z   7z archive
    //-m0=lzma2 lzma2 method
    //-mx=9  level of compression = 9 (Ultra)
    //-mfb=64 number of fast bytes for LZMA = 64
    //-md=32m dictionary size = 32 megabytes
    //-ms=on solid archive = on
    //TODO: mx->9
    runCmd(SZIP_EXECUTABLE + " a -r -y -t7z -m0=lzma2 -mx=0 -mfb=64 -md=32m -ms=on "
           + QDir::toNativeSeparators(dir) + " "
           + QDir::toNativeSeparators(archivePath));
}

bool DeltaPatcher::createDir(const QDir& dir) const
{
    if (dir.exists())
    {
        DBG << "ERROR:" << dir.absolutePath() << "already exists! Deleting..";
        //if (dir.removeRecursively())
        //{ TODO Uncomment
        //    DBG << "ERROR: Unable to remove directory:" << dir.absolutePath();
        //    return false;
        //}
    }
    DBG << "Creating tmp directory:" << dir.absolutePath();
    if (!dir.mkpath(SEPARATOR))
    {
        DBG << "ERROR: Unable to create tmp dir:" << dir.absolutePath();
        return false;
    }
    return true;
}

int DeltaPatcher::latestVersion(const QString& modName) const
{
    QDir patchesDir = QDir(patchesFi_->absoluteFilePath());
    QStringList fileNames = patchesDir.entryList(QDir::Files);
    return latestVersion(modName, fileNames);
}

int DeltaPatcher::latestVersion(const QString& modName, const QStringList& fileNames)
{
    int rVal = -1;
    for (QString fileName : fileNames)
    {
        if (!fileName.contains(QRegExp(modName + ".*7z")))
            continue;

        //@mod.1.2133.7z
        int version = fileName.split(SEPARATOR).at(1).toInt();
        rVal = std::max(rVal, version);
    }
    return rVal;
}
