#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QStringLiteral>
#include <QTimer>

#include "../../afisynclogger.h"
#include "../../fileutils.h"
#include "ahasher.h"
#include "deltapatcher.h"

using namespace Qt::StringLiterals;

#ifdef Q_OS_LINUX
const QString DeltaPatcher::XDELTA_EXECUTABLE = u"xdelta3"_s;
const QString DeltaPatcher::SZIP_EXECUTABLE = u"7za"_s;
#else
    const QString DeltaPatcher::XDELTA_EXECUTABLE = "bin\\xdelta3.exe";
    const QString DeltaPatcher::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

//20 minutes. 7za compression may take a while
const int DeltaPatcher::TIMEOUT = 20*60000;
const QString DeltaPatcher::DELTA_POSTFIX = u"_patch"_s;
const QString DeltaPatcher::DELTA_EXTENSION = u".vcdiff"_s;
const QString DeltaPatcher::SEPARATOR = u"."_s;
const QString DeltaPatcher::PATCH_DIR = u"delta_patch"_s;

DeltaPatcher::DeltaPatcher(const QString& patchesPath):
    QObject(nullptr),
    bytesPatched_(0),
    extractingPatches_(false),
    totalBytes_(-1),
    patchesFi_(new QFileInfo(patchesPath)),
    console_(new Console(this))
{
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
}

DeltaPatcher::~DeltaPatcher()
{
    delete patchesFi_;
}

void DeltaPatcher::stop()
{
    console_->terminate();
}

void DeltaPatcher::patch(const QString& modPath)
{
    QMetaObject::invokeMethod(this, [=]
    {
        patchDirSync(modPath);
    }, Qt::QueuedConnection);
}

void DeltaPatcher::patchDirSync(const QString& modPath)
{
    QDir patchesDir = QDir(patchesFi_->absoluteFilePath());
    const QStringList allPatches = patchesDir.entryList(QDir::Files);
    Q_ASSERT(!allPatches.isEmpty());
    QString modName = QFileInfo(modPath).fileName();

    mutex_.lock();
    patchingMod_ = modName;
    mutex_.unlock();

    QStringList patches = filterPatches(modPath, allPatches);
    if (patches.isEmpty())
    {
        LOG_ERROR << "No patches found for " << modName << " from "
            << allPatches << " hash = " << AHasher::hash(modPath);
        emit patched(modPath, false);
        return;
    }
    LOG << "Patching " << modName << " with " << patches << " ...";
    applyPatches(modPath, patches);
}

//Recursively applies all patches to modPath
void DeltaPatcher::applyPatches(const QString& modPath, QStringList patches)
{
    while (!patches.isEmpty())
    {
        auto patchFileName = patches.takeFirst();
        bool result = patch(patchFileName, modPath);
        Q_ASSERT(result);
    }
    //SUCCESS
    emit patched(modPath, true);
    bytesPatched_ = 0;

    mutex_.lock();
    patchingMod_.clear();
    mutex_.unlock();
}

QStringList DeltaPatcher::filterPatches(const QString& modPath, const QStringList& allPatches)
{
    const QString modName = QFileInfo(modPath).fileName();
    const int ltstVersion = latestVersion(modName, allPatches);
    if (ltstVersion == -1)
    {
        //Function used for patchAvailable() so no error is printed here.
        return {};
    }
    QStringList patches;
    const QString hash = AHasher::hash(modPath);
    LOG << "Calculated hash for " << modPath << ". Result: " << hash;
    const QRegularExpression regEx(modName + ".*" + hash + "\\" + SEPARATOR + "7z");
    const QStringList matches = allPatches.filter(regEx);
    if (matches.isEmpty())
    {
        return {};
    }

    const QString& patchName = matches.at(0);
    // First version
    int version = patchName.split(SEPARATOR).at(1).toInt();
    patches.append(patchName);
    while (version != ltstVersion)
    {
        ++version;
        QRegularExpression regExp(modName + "\\." + QString::number(version) + ".*7z");
        QStringList matches = allPatches.filter(regExp);
        if (matches.size() != 1)
        {
            LOG_ERROR << "Incorrect number (" << matches.size()
                     << ") of suitable patches. Aborting...2";
            return {};
        }
        patches.append(matches.at(0));
    }
    return patches;
}

qint64 DeltaPatcher::bytesPatched(const QString& modName)
{
    if (patching(modName))
    {
        return bytesPatched_;
    }

    return 0;
}

qint64 DeltaPatcher::totalBytes(const QString& modName)
{
    if (patching(modName))
    {
        return totalBytes_;
    }

    return -1;
}

bool DeltaPatcher::patching(const QString& modName)
{
    mutex_.lock();
    const bool retVal = patchingMod_ == modName;
    mutex_.unlock();
    return retVal;
}

bool DeltaPatcher::isPatching()
{
    return !patchingMod_.isEmpty();
}

//Synchronous patch function
bool DeltaPatcher::patch(const QString& patch, const QString& modPath)
{
    LOG << "Patching " << modPath << " with " << patch;
    if (!extract(patchesFi_->absoluteFilePath() + "/" + patch))
    {
        return false;
    }

    return patchExtracted(patchesFi_->absoluteFilePath() + "/" + PATCH_DIR, modPath);
}

bool DeltaPatcher::patchAbsolutePath(const QString& patch, const QString& modPath)
{
    LOG << "Patching " << modPath << " with " << patch;
    QFileInfo(patch).absoluteFilePath();
    if (!extract(patch))
    {
        return false;
    }

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
        QFileInfo patchFile = it.nextFileInfo();
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
            patchesDir.entryList().filter(QRegularExpression(modName + "\\..*\\." + oldHash + "\\.7z" ));
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
        QFileInfo newFile = it.nextFileInfo();

        QString relPath = newFile.absoluteFilePath().remove(newModPath);
        if (relPath.startsWith('/')) {
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

    QMetaObject::invokeMethod(this, [=]
    {
        compress(patchPath, deltaPath);
    }, Qt::BlockingQueuedConnection);

    FileUtils::safeRemoveRecursively(deltaDir);

    //Verify that the latest version doesn't get delta patched
    //due to hash collision.
    QStringList deletedFiles = removePatchesFromLatest(newModPath, patchesPath);

    return rVal && !deletedFiles.contains(patchName);
}

QStringList DeltaPatcher::removePatchesFromLatest(const QString& latestPath, const QString& patchesPath)
{
    QString hash = AHasher::hash(latestPath);
    QString modName = QFileInfo(latestPath).fileName();
    QRegularExpression regEx(modName + ".*" + hash + ".*\\.7z");
    const QStringList matches = QDir(patchesPath).entryList().filter(regEx);
    QStringList rVal;
    rVal.reserve(matches.size());
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
bool DeltaPatcher::createEmptyDir(QDir dir)
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
    if (!dir.mkpath(u"."_s)) {
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
        static const QRegularExpression exp{modName + u".*7z"_s};
        if (!fileName.contains(exp))
        {
            continue;
        }

        //@mod.1.2133.7z
        int version = fileName.split(SEPARATOR).at(1).toInt();
        rVal = std::max(rVal, version);
    }
    return rVal;
}
