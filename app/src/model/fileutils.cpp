#include <fstream>

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QStringLiteral>

#include "afisynclogger.h"
#include "fileutils.h"
#include "paths.h"
#include "settingsmodel.h"

using namespace Qt::StringLiterals;


namespace {
    QStringList safeSubpaths_;

    bool pathIsSafe(const QString& path)
    {
        static SettingsModel& settings = settings.instance();

        QString pathFull = QFileInfo(path).absoluteFilePath();
        QString pathUpper = pathFull.toUpper();
        QStringList safeSubpaths;
        safeSubpaths.append(settings.modDownloadPath());
        safeSubpaths.append(settings.arma3Path());
        safeSubpaths.append(settings.teamSpeak3Path());
        safeSubpaths.append(Paths::teamspeak3AppDataPath());
        safeSubpaths.append(QCoreApplication::applicationDirPath());
        safeSubpaths.append(u"."_s);
        safeSubpaths.append(safeSubpaths_);

        for (const QString& safeSubpath : safeSubpaths)
        {
            QString safeUpper = QFileInfo(safeSubpath).absoluteFilePath().toUpper();
            //Shortest possible save path: C:/d (4 characters)
            if (safeUpper.length() >= 4 && pathUpper.startsWith(safeUpper))
            {
                return true;
            }
        }
        LOG_ERROR << pathFull << " not in safe subpaths " << safeSubpaths << ". Operation aborted!";
        return false;
    }
}

//https://qt.gitorious.org/qt-creator/qt-creator/source/1a37da73abb60ad06b7e33983ca51b266be5910e:src/app/main.cpp#L13-189
// taken from utils/fileutils.cpp. We can not use utils here since that depends app_version.h.
bool FileUtils::copy(const QString& srcPath, const QString& dstPath)
{
    QFileInfo srcFi(srcPath);
    //Directory
    if (srcFi.isDir())
    {
        QDir().mkpath(dstPath);
        if (!QFile::exists(dstPath))
        {
            LOG_ERROR << "Failed to create directory" << dstPath;
            return false;
        }
        QDir srcDir(srcPath);
        const QStringList fileNames = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString& fileName : fileNames)
        {
            const QString newSrcPath = srcPath + '/' + fileName;
            const QString newDstPath = dstPath + '/' + fileName;
            if (!copy(newSrcPath, newDstPath))
            {
                return false;
            }
        }
    }
    //File
    else if (srcFi.isFile())
    {
        QFile dstFile(dstPath);
        FileUtils::safeRemove(dstFile);
        LOG << "Copying " << srcPath << " to " << dstPath;
        if (dstFile.exists())
        {
            LOG << "Cannot overwrite file " << dstPath;
            return false;
        }
        if (!QFile::copy(srcPath, dstPath))
        {
            LOG << "Failure to copy " << srcPath << " to " << dstPath;
            return false;
        }
    }
    else
    {
        LOG << srcPath << " does not exist";
        return false;
    }
    return true;
}

bool FileUtils::move(const QString& srcPath, const QString& dstPath)
{
    QFileInfo srcFi(srcPath);
    //Directory
    if (srcFi.isDir())
    {
        QDir().mkpath(dstPath);
        if (!QFile::exists(dstPath))
        {
            LOG_ERROR << "Failed to create directory " << dstPath;
            return false;
        }
        QDir srcDir(srcPath);
        QStringList fileNames = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString& fileName : fileNames)
        {
            const QString newSrcPath = srcPath + '/' + fileName;
            const QString newDstPath = dstPath + '/' + fileName;
            if (!move(newSrcPath, newDstPath))
            {
                return false;
            }
        }
    }
    //File
    else if (srcFi.isFile())
    {
        QFile dstFile(dstPath);
        LOG << "Moving " << srcPath << " to " << dstPath;
        FileUtils::safeRemove(dstFile);
        if (dstFile.exists())
        {
            LOG << "Cannot overwrite file " << dstPath;
            return false;
        }
        if (!safeRename(srcPath, dstPath))
        {
            LOG << "Failure to copy " << srcPath << " to " << dstPath;
            return false;
        }
    }
    else
    {
        LOG << srcPath << " does not exist";
        return false;
    }
    return true;
}

qint64 FileUtils::dirSize(const QString& path)
{
    qint64 rVal = 0;
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        rVal += it.fileInfo().size();
        it.next();
    }

    return rVal;
}

bool FileUtils::rmCi(const QString& path)
{
    QString cPath = casedPath(path);
    if (cPath.length() <= 3)
    { //D:/
        return false;
    }

    return safeRemove(cPath);
}

QByteArray FileUtils::readFile(const QString& path)
{
    QByteArray rVal;
    QFile file(path);

    file.open(QFile::ReadOnly);
    if (!file.isReadable())
    {
        LOG_WARNING << "Unable to read file: " << path;
        return rVal;
    }

    rVal = file.readAll();
    file.close();
    return rVal;
}

//Writes a new file
bool FileUtils::writeFile(const QByteArray& data, const QString& path)
{
    QFile file(path);
    QDir parentDir = QFileInfo(path).dir();
    parentDir.mkpath(u"."_s);
    safeRemove(file);
    file.open(QFile::WriteOnly);

    if (!file.isWritable())
    {
        LOG_ERROR << "File " << path << " is not writable.";
        return false;
    }
    LOG << "Writing file: " << path;
    file.write(data);
    file.close();
    return true;
}

QString FileUtils::casedPath(const QString& path)
{
    QString pathCi = QFileInfo(path).absoluteFilePath();
    //Construct case sentive path by comparing file or dir names to case sentive ones.
    QStringList ciNames = pathCi.split('/');
    QString casedPath = ciNames.first().endsWith(':') ? ciNames.first() + "/"
                                                                     : u"/"_s;
    ciNames.removeFirst(); //Drive letter on windows or "" on linux.

    for (const QString& ciName : ciNames)
    {
        QDirIterator it(casedPath);
        while (true)
        {
            if (!it.hasNext())
            {
                return {};
            }
            QFileInfo fi = it.nextFileInfo();
            if (fi.fileName().toUpper() == ciName.toUpper())
            {
                casedPath += (casedPath.endsWith('/') ? "" : "/") + fi.fileName();
                break;
            }
        }
    }
    return casedPath;
}

bool FileUtils::filesIdentical(const QString& path1, const QString& path2)
{
    std::ifstream f1(path1.toStdString(), std::ifstream::binary|std::ifstream::ate);
    std::ifstream f2(path2.toStdString(), std::ifstream::binary|std::ifstream::ate);

    if (f1.fail() || f2.fail()) {
      return false; //file problem
    }

    if (f1.tellg() != f2.tellg()) {
      return false; //size mismatch
    }

    //seek back to beginning and use std::equal to compare contents
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(f2.rdbuf()));
}

bool FileUtils::safeRename(const QString& srcPath, const QString& dstPath)
{
    if (!pathIsSafe(srcPath))
    {
        return false;
    }

    return QFile::rename(srcPath, dstPath);
}

void FileUtils::safeRemoveEmptyDirs(const QString& path)
{
    QDirIterator it(path, QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext())
    {
        const QString subPath = it.next();
        safeRemoveEmptyDirs(subPath);
        QDir dir(subPath);
        if (dir.isEmpty())
        {
            safeRemoveRecursively(subPath);
        }
    }
}

bool FileUtils::safeRemove(const QString& filePath)
{
    QFile file(filePath);
    return safeRemove(file);
}

bool FileUtils::safeRemove(QFile& file)
{
    QString path = QFileInfo(file).absoluteFilePath();

    if (pathIsSafe(path) && file.remove())
    {
        LOG << "Removed " << path;
        return true;
    }
    return false;
}

bool FileUtils::safeRemoveRecursively(QDir& dir)
{
    QString path = dir.absolutePath();

    if (pathIsSafe(path) && dir.removeRecursively())
    {
        LOG << "Removed " << path;
        return true;
    }
    return false;
}

bool FileUtils::safeRemoveRecursively(const QString& path)
{
    QDir dir = QDir(path);
    return safeRemoveRecursively(dir);
}

void FileUtils::appendSafePath(const QString& path)
{
    safeSubpaths_.append(path);
}
