#include <QFileInfo>
#include <QTimer>
#include <QStringList>
#include <QDirIterator>
#include <QRegExp>
#include <QDebug>
#include "../../fileutils.h"
#include "../../debug.h"
#include "../../runningtime.h"
#include "../../global.h"
#include "ahasher.h"
#include "deltapatcher.h"

#ifdef Q_OS_LINUX
    const QString DeltaPatcher::XDELTA_EXECUTABLE = "xdelta3";
    const QString DeltaPatcher::SZIP_EXECUTABLE = "7za";
#else
    const QString DeltaPatcher::XDELTA_EXECUTABLE = "bin\\xdelta3.exe";
    const QString DeltaPatcher::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

//20 minutes. 7za compression may take a while
const int DeltaPatcher::TIMEOUT = 20*60000;
const QString DeltaPatcher::DELTA_POSTFIX = "_patch";
const QString DeltaPatcher::DELTA_EXTENSION = ".vcdiff";
const QString DeltaPatcher::SEPARATOR = ".";
const QString DeltaPatcher::PATCH_DIR = "delta_patch";


DeltaPatcher::DeltaPatcher(const QString& patchesPath):
    QObject(nullptr),
    bytesPatched_(0),
    totalBytes_(0)
{
    //Required because patching process is ran
    //synchronously but still needs to be shutdownable.
    moveToThread(&thread_);
    thread_.setObjectName("DeltaPatcher Thread");
    thread_.start();
    QMetaObject::invokeMethod(this, "threadConstructor", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, patchesPath));
}

DeltaPatcher::~DeltaPatcher()
{
    thread_.quit();
    thread_.wait(1000);
    thread_.terminate();
    thread_.wait(1000);
}

void DeltaPatcher::threadConstructor(const QString& patchesPath)
{
    patchesFi_ = new QFileInfo(patchesPath);
    console_ = new Console(this);
}

void DeltaPatcher::stop()
{
    if (thread_.isRunning())
    {
        console_->terminate();
        thread_.terminate();
    }
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
    patchingMod_ = modName; //Needed for eta
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
            bytesPatched_ = 0;
            patchingMod_ = "";
            return;
        }
    }
    emit patched(modPath, true);
    bytesPatched_ = 0;
    patchingMod_ = "";
}

QStringList DeltaPatcher::filterPatches(const QString& modPath, const QStringList& allPatches)
{
    static const QString ERROR_NO_PATCHES = "ERROR: no patches found for";

    QString modName = QFileInfo(modPath).fileName();
    int ltstVersion = latestVersion(modName, allPatches);
    if (ltstVersion == -1)
    {
        //Function used for patchAvailable() so no error is printed here.
        return QStringList();
    }
    QStringList patches;
    QString hash = AHasher::hash(modPath);
    QRegExp regEx(modName + ".*" + hash + "\\" + SEPARATOR + "7z");
    QStringList matches = allPatches.filter(regEx);
    if (matches.size() != 1)
    {
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
    return patches;
}

qint64 DeltaPatcher::bytesPatched(const QString& modName) const
{
    static const qint64 INTERVAL = 60000;

    static qint64 prevBytesPatched = 0;
    static qint64 speed = 250000;
    static qint64 increment = 0;
    static qint64 startTime = runningTimeMs();
    static qint64 prevTime = runningTimeMs();

    if (modName.toLower() != patchingMod_.toLower())
        return 0;

    qint64 currentTime = runningTimeMs();
    //Simulates progression, when no actual change happens
    //because user needs continious feedback.
    if (prevBytesPatched == bytesPatched_)
    {
        increment += (speed * (currentTime - prevTime))/1000;
        prevTime = currentTime;
        qint64 rVal =  bytesPatched_ + increment;
        DBG << rVal;
        return rVal;
    }
    //Step completed, re-estimate speed.
    qint64 dTime = currentTime - startTime;
    if (dTime > INTERVAL)
    {
        speed = (bytesPatched_ - prevBytesPatched) / dTime;
        startTime = currentTime;
        DBG << "New patching speed estimation:" << speed << "bytes per second.";
    }
    increment = 0;
    prevBytesPatched = bytesPatched_;
    return std::min(bytesPatched_, totalBytes_);
}

qint64 DeltaPatcher::totalBytes(const QString& modName) const
{
    if (patchingMod_.toLower() == modName.toLower())
        return totalBytes_;

    return 0;
}

bool DeltaPatcher::notPatching()
{
    return patchingMod_.size() == 0;
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

//ToDo: Use static dirs
void DeltaPatcher::cleanUp(QDir& deltaDir, QDir& tmpDir)
{
    DBG << "Deleting dir" << deltaDir.absolutePath();
    FileUtils::safeRemoveRecursively(deltaDir);
    DBG << "Deleting dir" << tmpDir.absolutePath();
    FileUtils::safeRemoveRecursively(tmpDir);
}

bool DeltaPatcher::patchExtracted(const QString& extractedPath, const QString& targetPath)
{
    QDir deltaDir = QDir(extractedPath);
    totalBytes_ = FileUtils::dirSize(extractedPath);
    QDir tmpDir = QDir(extractedPath + "_tmp");

    if (!createEmptyDir(tmpDir))
    {
        cleanUp(deltaDir, tmpDir);
        return false;
    }

    DBG << "extractedPath" << extractedPath;
    QDirIterator it(extractedPath, QDir::Files, QDirIterator::Subdirectories);
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
            DBG << "Moving" << diffPath << "to" << patchedPath;
            QFile::rename(diffPath, patchedPath);
            continue;
        }
        QString targetFilePath = targetPath + relPath;

        bool rVal = console_->runCmd(XDELTA_EXECUTABLE + " -d -s " + " \""
                           + QDir::toNativeSeparators(targetFilePath) + "\" \""
                           + QDir::toNativeSeparators(diffPath) + "\" \""
                           + QDir::toNativeSeparators(patchedPath)) + "\"";

        if (!rVal)
        {
            DBG << "Warning: Delta patching failed.";
            cleanUp(deltaDir, tmpDir);
            return false;
        }
        bytesPatched_ += patchFile.size();
    }
    FileUtils::move(tmpDir.absolutePath(), targetPath);
    cleanUp(deltaDir, tmpDir);
    DBG << "Delta patching succesfull!";
    return true;
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
        FileUtils::safeRemoveRecursively(deltaDir);
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
        if (oldFile.size() == QFileInfo(laterPath).size())
        {
            DBG << "Files" << oldPath << "and file" << laterPath << "are (size) identical. Delta patch generation aborted.";
            continue;
        }
        //Creates uncompressed delta patch file
        QMetaObject::invokeMethod(console_, "runCmd", Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, XDELTA_EXECUTABLE + " -e -S none -s \""
                                        + oldPath + "\" \"" + laterPath + "\" \"" + outputPath + "\""));
        DBG << "New delta patch file generated" << outputPath;
    }

    QMetaObject::invokeMethod(this, "compress", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, patchPath), Q_ARG(QString, deltaPath));

    DBG << "Deleting directory" << deltaPath;
    FileUtils::safeRemoveRecursively(deltaDir);

    return true;
}

bool DeltaPatcher::extract(const QString& zipPath)
{
    DBG << "Extracting" << zipPath;
    QFileInfo fi = QFileInfo(zipPath);
    return console_->runCmd(SZIP_EXECUTABLE + " -y x \""
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
    //-mx=9 compression level
    console_->runCmd(SZIP_EXECUTABLE + " a -r -y -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on "
           + QDir::toNativeSeparators(dir) + " "
           + QDir::toNativeSeparators(archivePath));
}

//Creates clean empty directory
bool DeltaPatcher::createEmptyDir(QDir dir) const
{
    if (dir.exists())
    {
        DBG << "ERROR:" << dir.absolutePath() << "already exists! Deleting..";
        if (!FileUtils::safeRemoveRecursively(dir))
        {
            DBG << "ERROR: Unable to remove directory:" << dir.absolutePath();
            return false;
        }
    }
    DBG << "Creating directory:" << dir.absolutePath();
    if (!dir.mkpath("."))
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
