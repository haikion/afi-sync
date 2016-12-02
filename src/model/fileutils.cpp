#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QDirIterator>
#include <QCoreApplication>
#include "debug.h"
#include "fileutils.h"
#include "settingsmodel.h"

QStringList FileUtils::safeSubpaths_;

//https://qt.gitorious.org/qt-creator/qt-creator/source/1a37da73abb60ad06b7e33983ca51b266be5910e:src/app/main.cpp#L13-189
// taken from utils/fileutils.cpp. We can not use utils here since that depends app_version.h.
bool FileUtils::copy(const QString& srcPath, const QString& dstPath)
{
    QFileInfo srcFi(srcPath);
    //Directory
    if (srcFi.isDir())
    {
        QDir().mkpath(dstPath);
        if (!QFileInfo(dstPath).exists())
        {
            DBG << "ERROR: Failed to create directory" << dstPath;
            return false;
        }
        QDir srcDir(srcPath);
        QStringList fileNames = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString& fileName : fileNames)
        {
            const QString newSrcPath
                    = srcPath + QLatin1Char('/') + fileName;
            const QString newDstPath
                    = dstPath + QLatin1Char('/') + fileName;
            if (!copy(newSrcPath, newDstPath))
                return false;
        }
    }
    //File
    else if (srcFi.isFile())
    {
        QFile dstFile(dstPath);
        DBG << "Copying" << srcPath << "to" << dstPath;
        FileUtils::safeRemove(dstFile);
        if (dstFile.exists())
        {
            DBG << "Cannot overwrite file" << dstPath;
            return false;
        }
        if (!QFile::copy(srcPath, dstPath))
        {
            DBG << "Failure to copy " << srcPath << "to" << dstPath;
            return false;
        }
    }
    else
    {
        DBG << srcPath << "does not exist";
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
        if (!QFileInfo(dstPath).exists())
        {
            DBG << "ERROR: Failed to create directory" << dstPath;
            return false;
        }
        QDir srcDir(srcPath);
        QStringList fileNames = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        for (const QString& fileName : fileNames)
        {
            const QString newSrcPath
                    = srcPath + QLatin1Char('/') + fileName;
            const QString newDstPath
                    = dstPath + QLatin1Char('/') + fileName;
            if (!move(newSrcPath, newDstPath))
                return false;
        }
    }
    //File
    else if (srcFi.isFile())
    {
        QFile dstFile(dstPath);
        DBG << "Moving" << srcPath << "to" << dstPath;
        FileUtils::safeRemove(dstFile);
        if (dstFile.exists())
        {
            DBG << "Cannot overwrite file" << dstPath;
            return false;
        }
        if (!safeRename(srcPath, dstPath))
        {
            DBG << "Failure to copy " << srcPath << "to" << dstPath;
            return false;
        }
    }
    else
    {
        DBG << srcPath << "does not exist";
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

bool FileUtils::rmCi(QString path)
{
    QString cPath = casedPath(path);
    if (cPath.length() <= 3) //D:/
        return false;

    return safeRemove(cPath);
}

QByteArray FileUtils::readFile(const QString& path)
{
    QByteArray rVal;
    QFile file(path);

    file.open(QFile::ReadOnly);
    if (!file.isReadable())
    {
        DBG << "Warning: unable to read file" << path;
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
    parentDir.mkpath(".");
    safeRemove(file);
    file.open(QFile::WriteOnly);

    if (!file.isWritable())
    {
        DBG << "ERROR: file" << path << " is not writable.";
        return false;
    }
    DBG << "Writing file:" << path;
    file.write(data);
    file.close();
    return true;
}

QString FileUtils::casedPath(const QString& path)
{
    DBG << "Input:" << path;
    QString pathCi = QFileInfo(path).absoluteFilePath();
    //Construct case sentive path by comparing file or dir names to case sentive ones.
    QStringList ciNames = pathCi.split("/");
    QString casedPath = ciNames.first().endsWith(":") ? ciNames.first() + "/" : "/";
    ciNames.removeFirst(); //Drive letter on windows or "" on linux.

    for (const QString& ciName : ciNames)
    {
        bool failed = true;
        QDirIterator it(casedPath);
        while (it.hasNext())
        {
            QFileInfo fi = it.next();
            if (fi.fileName().toUpper() == ciName.toUpper())
            {
                casedPath += (casedPath.endsWith("/") ? "" : "/") + fi.fileName();
                DBG << casedPath;
                failed = false;
            }
        }
        if (failed)
        {
            DBG << "ERROR: Unable to construct case sensitive path from" << path;
            return QString();
        }
    }
    DBG << "Output:" << casedPath;
    return casedPath;
}

bool FileUtils::safeRename(const QString& srcPath, const QString& dstPath)
{
    if (!pathIsSafe(srcPath))
        return false;

    DBG << "Rename" << srcPath << "to"  << dstPath;
    return QFile::rename(srcPath, dstPath);
}

bool FileUtils::safeRemove(const QString& filePath)
{
    QFile file(filePath);
    return safeRemove(file);
}

bool FileUtils::safeRemove(QFile& file)
{
    QString path = QFileInfo(file).absoluteFilePath();
    if (!pathIsSafe(path))
        return false;

    DBG << "Removing" << path;
    return file.remove();
}

bool FileUtils::safeRemoveRecursively(QDir& dir)
{
    QString path = dir.absolutePath();
    if (!pathIsSafe(path))
        return false;

    DBG << "Removing" << path;
    return dir.removeRecursively();
}

bool FileUtils::safeRemoveRecursively(const QString& path)
{
    QDir dir = QDir(path);
    return safeRemoveRecursively(dir);
}

bool FileUtils::pathIsSafe(const QString& path)
{
    QString pathUpper = path.toUpper();
    QStringList safeSubpaths;
    safeSubpaths.append(SettingsModel::modDownloadPath());
    safeSubpaths.append(SettingsModel::arma3Path());
    safeSubpaths.append(SettingsModel::teamSpeak3Path());
    safeSubpaths.append(QCoreApplication::applicationDirPath());
    safeSubpaths.append(".");
    safeSubpaths.append(safeSubpaths_);

    for (QString safeSubpath : safeSubpaths)
    {
        QString safeUpper = QFileInfo(safeSubpath).absoluteFilePath().toUpper();
        //Shortest possible save path: C:/d (4 characters)
        if (safeUpper.length() >= 4 && pathUpper.startsWith(safeUpper))
            return true;
    }

    DBG << "ERROR: " << path << "not in safe subpaths" << safeSubpaths << ". Operation aborted!";
    return false;
}

void FileUtils::appendSafePath(const QString& path)
{
    safeSubpaths_.append(path);
}
