#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegExp>
#include <QStringList>
#include <QTimer>
#include "libtorrent/torrent_status.hpp"
#include "../../fileutils.h"
#include "../../global.h"
#include "../../runningtime.h"
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

DeltaPatcher::DeltaPatcher(const QString& patchesPath, const libtorrent::torrent_handle& handle):
    QObject(nullptr),
    bytesPatched_(0),
    extractingPatches_(false),
    totalBytes_(-1),
    handle_(handle)
{
    //Required because patching process is ran
    //synchronously but still needs to be shutdownable.
    moveToThread(&thread_);
    thread_.setObjectName("DeltaPatcher Thread");
    thread_.start();
    QMetaObject::invokeMethod(this, "threadConstructor", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, patchesPath));
}

// Used for command line interface. Does not use worker thread
// TODO: Separate synchronous operations into different class and use that class instead
// with command line
DeltaPatcher::DeltaPatcher():
    QObject(nullptr),
    bytesPatched_(0),
    extractingPatches_(false),
    totalBytes_(-1)
{
    threadConstructor("");
}


DeltaPatcher::~DeltaPatcher()
{
    thread_.quit();
    thread_.wait(1000);
    thread_.terminate();
    thread_.wait();
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
    QMetaObject::invokeMethod(this, "patchDirSync", Qt::QueuedConnection, Q_ARG(QString, modPath));
}

void DeltaPatcher::patchDirSync(const QString& modPath)
{
    QDir patchesDir = QDir(patchesFi_->absoluteFilePath());
    const QStringList allPatches = patchesDir.entryList(QDir::Files);
    LOG << allPatches << " " << patchesDir.absolutePath()
        << " " << patchesFi_->absoluteFilePath() << modPath;
    QString modName = QFileInfo(modPath).fileName();

    mutex_.lock();
    patchingMod_ = modName;
    mutex_.unlock();

    QStringList patches = filterPatches(modPath, allPatches);
    if (patches.isEmpty())
    {
        LOG_ERROR << "no patches found for" << modName << "from"
            << allPatches << "hash =" << AHasher::hash(modPath);
        emit patched(modPath, false);
        return;
    }
    applyPatches(modPath, patches, 0);
}

//Recursively applies all patches to modPath
//
//libTorrent may report downloaded bytes early.
//which means AFISync may try apply the patch before it is actually downloaded
//This functions tries to wait for the libTorrent to actually finish the download.
void DeltaPatcher::applyPatches(const QString& modPath, QStringList patches, int attempts)
{
    static const int MAX_ATTEMPTS = 10;

    if (!handle_.status().is_finished)
    {
        // Wait for torrent to finish (Might occur if recheck was needed)
        QTimer::singleShot(10000, this, [=] {applyPatches(modPath, patches, attempts);});
        LOG << "Waiting for 10 s for patches torrent to finish. modPath = " << modPath << " patches = "
            << patches << " attempts = " << attempts << "/" << MAX_ATTEMPTS;
        return;
    }

    if (attempts > MAX_ATTEMPTS)
    {
        //FAIL
        emit patched(modPath, false);
        bytesPatched_ = 0;

        mutex_.lock();
        patchingMod_ = "";
        mutex_.unlock();

        return;
    }
    if (patch(patches.first(), modPath))
    {
        patches.takeFirst();
        if (patches.isEmpty())
        {
            //SUCCESS
            emit patched(modPath, true);
            bytesPatched_ = 0;

            mutex_.lock();
            patchingMod_ = "";
            mutex_.unlock();

            return;
        }
        applyPatches(modPath, patches, attempts);
    }
    else
    {
        //Try as long as downloading and after that 10 times with 10s delays.
        if (handle_.status().state == libtorrent::torrent_status::state_t::finished)
        {
            handle_.force_recheck();
            ++attempts;
        }
        QTimer::singleShot(10000, this, [=] {applyPatches(modPath, patches, attempts);});
    }
}

// TODO: Move to DeltaUtils
QStringList DeltaPatcher::filterPatches(const QString& modPath, const QStringList& allPatches)
{
    const QString modName = QFileInfo(modPath).fileName();
    const int ltstVersion = latestVersion(modName, allPatches);
    if (ltstVersion == -1)
    {
        //Function used for patchAvailable() so no error is printed here.
        return QStringList();
    }
    QStringList patches;
    const QString hash = AHasher::hash(modPath);
    LOG << "Calculated hash for " << modPath << ". Result: " << hash;
    const QRegExp regEx(modName + ".*" + hash + "\\" + SEPARATOR + "7z");
    const QStringList matches = allPatches.filter(regEx);
    if (matches.isEmpty())
        return QStringList();

    const QString& patchName = matches.at(0);
    //First version. TODO: Why not just match hashes in recursive manner?
    int version = patchName.split(SEPARATOR).at(1).toInt();
    patches.append(patchName);
    while (version != ltstVersion)
    {
        ++version;
        QRegExp regExp(modName + "\\." + QString::number(version) + ".*7z");
        QStringList matches = allPatches.filter(regExp);
        if (matches.size() != 1)
        {
            LOG_ERROR << "Incorrect number (" << matches.size()
                     << ") of suitable patches. Aborting...2";
            return QStringList();
        }
        patches.append(matches.at(0));
    }
    return patches;
}

qint64 DeltaPatcher::bytesPatched(const QString& modName)
{
    if (patching(modName))
        return bytesPatched_;

    return 0;
}

qint64 DeltaPatcher::totalBytes(const QString& modName)
{
    if (patching(modName))
        return totalBytes_;

    return -1;
}

bool DeltaPatcher::patching(const QString& modName)
{
    mutex_.lock();
    const bool retVal = patchingMod_ == modName;
    mutex_.unlock();
    return retVal;
}

bool DeltaPatcher::notPatching()
{
    mutex_.lock();
    const bool retVal = patchingMod_.size() == 0;
    mutex_.unlock();
    return retVal;
}

//Synchronous patch function
bool DeltaPatcher::patch(const QString& patch, const QString& modPath)
{
    LOG << "Patching " << modPath << " with " << patch;
    if (!extract(patchesFi_->absoluteFilePath() + "/" + patch))
        return false;

    return patchExtracted(patchesFi_->absoluteFilePath() + "/" + PATCH_DIR, modPath);
}

bool DeltaPatcher::patchAbsolutePath(const QString& patch, const QString& modPath)
{
    LOG << "Patching " << modPath << " with " << patch;
    QFileInfo(patch).absoluteFilePath();
    if (!extract(patch))
        return false;

    return patchExtracted(QFileInfo(patch).dir().absolutePath() + "/" + PATCH_DIR, modPath);
}

void DeltaPatcher::cleanUp(QDir& deltaDir, QDir& tmpDir)
{
    LOG << "Deleting dir " << deltaDir.absolutePath();
    FileUtils::safeRemoveRecursively(deltaDir);
    LOG << "Deleting dir " << tmpDir.absolutePath();
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

    LOG << "extractedPath " << extractedPath;
    QDirIterator it(extractedPath, QDir::Files, QDirIterator::Subdirectories);
    if (!it.hasNext())
    {
        LOG_ERROR << "Nothing to patch in " << extractedPath;
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
            LOG << "Moving " << diffPath << " to " << patchedPath;
            QFile::rename(diffPath, patchedPath);
            bytesPatched_ += patchFile.size();
            continue;
        }
        QString targetFilePath = targetPath + relPath;

        bool rVal = console_->runCmd(XDELTA_EXECUTABLE + " -d -s " + " \""
                           + QDir::toNativeSeparators(targetFilePath) + "\" \""
                           + QDir::toNativeSeparators(diffPath) + "\" \""
                           + QDir::toNativeSeparators(patchedPath) + "\"");

        if (!rVal)
        {
            LOG_WARNING << "Delta patching failed. targetPath = " << targetPath;
            cleanUp(deltaDir, tmpDir);
            return false;
        }
        bytesPatched_ += patchFile.size();
    }
    FileUtils::move(tmpDir.absolutePath(), targetPath);
    cleanUp(deltaDir, tmpDir);
    LOG << "Delta patching successful! targetPath = " << targetPath;
    return true;
}

bool DeltaPatcher::delta(const QString& oldModPath, QString newModPath)
{
    QString patchesPath = patchesFi_->absoluteFilePath();
    QDir patchesDir(patchesPath);
    QString deltaPath = patchesPath + "/" + PATCH_DIR;
    QDir deltaDir(deltaPath);
    QString oldHash = AHasher::hash(oldModPath);
    QString modName = QFileInfo(oldModPath).fileName();
    QString patchName = modName + SEPARATOR
            + QString::number(latestVersion(modName) + 1) + SEPARATOR + oldHash + ".7z";
    QString patchPath = patchesPath + "/" + patchName;

    QFileInfo laterFi = QFileInfo(newModPath);
    if (!laterFi.exists())
    {
        LOG_ERROR << "Path " << newModPath << " does not exist.";
        return false;
    }
    newModPath = laterFi.absoluteFilePath();

    QStringList conflictingFiles =
            patchesDir.entryList().filter(QRegExp(modName + "\\..*\\." + oldHash + "\\.7z" ));
    if (!conflictingFiles.isEmpty())
    {
        LOG_ERROR << "patch " << patchName << " collides with files: " << conflictingFiles;
        return false;
    }

    if (!patchesFi_->isWritable())
    {
        LOG_ERROR << "Directory " << patchesPath << " is not writable.";
        return false;
    }

    if (deltaDir.exists())
    {
        LOG_ERROR << "Directory " << deltaPath << " already exists. Deleting...";
        FileUtils::safeRemoveRecursively(deltaDir);
    }

    QDir().mkpath(deltaPath);
    QDirIterator it(newModPath, QDir::Files, QDirIterator::Subdirectories);
    bool rVal = false;
    while (it.hasNext())
    {
        QFileInfo newFile = it.next();

        QString relPath = newFile.absoluteFilePath().remove(newModPath);
        if (relPath.startsWith("/")) {
            relPath.remove(0, 1);
        }
        LOG << relPath << " " << newModPath;
        QFileInfo oldFile = QFileInfo(oldModPath + "/" + relPath);
        QString outputPath = deltaPath + "/" + relPath;
        QString parentPath = QFileInfo(outputPath).absolutePath();
        QString newPath = newFile.absoluteFilePath();

        // Do not create patch if path is differently cased
        // for example Addons/thing.pbo vs addons/thing.pbo
        auto casedPath = FileUtils::casedPath(oldFile.absoluteFilePath());
        if (!oldFile.exists() || !casedPath.contains(relPath))
        {
            LOG << "Copying " << newPath << " to " << outputPath;
            QFile::copy(newPath, outputPath);
            QDir().mkpath(parentPath);
            continue;
        }

        outputPath = outputPath + DELTA_EXTENSION;
        QString oldPath = QDir::toNativeSeparators(oldFile.absoluteFilePath());
        QString laterPath = QDir::toNativeSeparators(newPath);
        if (FileUtils::filesIdentical(oldPath, laterPath))
        {
            LOG << "Files " << oldPath << " and file " << laterPath << " are dentical. Delta patch generation aborted.";
            continue;
        }
        QDir().mkpath(parentPath);
        //Creates uncompressed delta patch file
        QMetaObject::invokeMethod(console_, "runCmd", Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, XDELTA_EXECUTABLE + " -e -S none -s \""
                                        + oldPath + "\" \"" + laterPath + "\" \"" + outputPath + "\""));
        LOG << "New delta patch file generated " << outputPath;
        rVal = true;
    }

    if (!rVal)
    {
        FileUtils::safeRemoveRecursively(deltaDir);
        LOG << "No patchable files found. Delta patch generation aborted.";
        return false;
    }

    QMetaObject::invokeMethod(this, "compress", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, patchPath), Q_ARG(QString, deltaPath));

    FileUtils::safeRemoveRecursively(deltaDir);

    //Verify that the latest version doesn't get delta patched
    //due to hash collision.
    QStringList deletedFiles = removePatchesFromLatest(newModPath, patchesPath);

    return rVal && !deletedFiles.contains(patchName);
}

QStringList DeltaPatcher::removePatchesFromLatest(const QString& latestPath, const QString& patchesPath) const
{
    QString hash = AHasher::hash(latestPath);
    QString modName = QFileInfo(latestPath).fileName();
    QRegExp regEx(modName + ".*" + hash + ".*\\.7z");
    QStringList matches = QDir(patchesPath).entryList().filter(regEx);
    QStringList rVal;
    for (const QString& name : matches)
    {
        FileUtils::safeRemove(patchesPath + "/" + name);
        LOG_ERROR << "Found patch from latest version. Patch " << name << " removed.";
        rVal.append(name);
    }

    return rVal;
}

bool DeltaPatcher::extract(const QString& zipPath)
{
    extractingPatches_ = true;
    LOG << "Extracting " << zipPath;
    QFileInfo fi = QFileInfo(zipPath);
    console_->runCmd(SZIP_EXECUTABLE + " -y x \""
           + QDir::toNativeSeparators(zipPath)
           + "\" -o\"" + QDir::toNativeSeparators(fi.absolutePath()) + "\"");
    extractingPatches_ = false;
    return QDir(fi.absolutePath() + "/" + PATCH_DIR).exists();
}

bool DeltaPatcher::patchExtracting()
{
    return extractingPatches_;
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
    console_->runCmd(SZIP_EXECUTABLE + " a -r -y -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on \""
           + QDir::toNativeSeparators(dir) + "\" \""
           + QDir::toNativeSeparators(archivePath) + "\"");
}

//Creates clean empty directory
bool DeltaPatcher::createEmptyDir(QDir dir) const
{
    if (dir.exists())
    {
        LOG_ERROR << dir.absolutePath() << " already exists! Deleting..";
        if (!FileUtils::safeRemoveRecursively(dir))
        {
            LOG_ERROR << "Unable to remove directory: " << dir.absolutePath();
            return false;
        }
    }
    LOG << "Creating directory: " << dir.absolutePath();
    if (!dir.mkpath("."))
    {
        LOG_ERROR << "Unable to create tmp dir: " << dir.absolutePath();
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
    for (const QString& fileName : fileNames)
    {
        if (!fileName.contains(QRegExp(modName + ".*7z")))
            continue;

        //@mod.1.2133.7z
        int version = fileName.split(SEPARATOR).at(1).toInt();
        rVal = std::max(rVal, version);
    }
    return rVal;
}
