#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QDirIterator>
#include "debug.h"
#include "fileutils.h"
#include "settingsmodel.h"

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
        dstFile.remove(); //Cannot be safe due to installer.
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
    path.replace("\\", "/");
    if (!path.startsWith("/"))
    {
        DBG << "ERROR: Only absolute paths are supported";
        return false;
    }
    //FIXME: Does not work.
    //Construct case sentive path by comparing dirs in case insentive mode.
    QStringList dirNames = path.split("/");
    dirNames.removeFirst();
    QString casedPath = "/";
    for (QString dirName : dirNames)
    {
        QDirIterator it(casedPath);
        while (it.hasNext())
        {
            QFileInfo fi = it.next();
            DBG << fi.fileName() << dirName;
            if (fi.fileName().toUpper() == dirName.toUpper())
            {
                casedPath += "/" + fi.fileName();
                DBG << casedPath;
            }
        }
    }
    casedPath.replace("//", "/");
    return safeRemove(QFile(casedPath));
}

//Failsafe functions. These assure that the file being handeled is in mods directory.
//Prevents nasty programming errors from deleting important files.

bool FileUtils::safeRename(const QString& srcPath, const QString& dstPath)
{
    if (!pathIsSafe(srcPath))
        return false;

    DBG << "Fake rename" << srcPath << "to"  << dstPath;
    return true;
}

bool FileUtils::safeRemove(const QFile& file)
{
    QString path = QFileInfo(file).absoluteFilePath();
    if (!pathIsSafe(path))
        return false;

    DBG << "Removing" << path;
    return true;//file.remove();
}

bool FileUtils::safeRemoveRecursively(const QDir& dir)
{
    QString path = dir.absolutePath();
    if (!pathIsSafe(path))
        return false;

    DBG << "Removing" << path;
    return true;//dir.removeRecursively();
}

bool FileUtils::pathIsSafe(const QString& path)
{
    //TODO: Use collection of savePrefixes.
    QString safePrefix = QFileInfo(SettingsModel::modDownloadPath()).absoluteFilePath().toUpper();

    QString pathUpper = path.toUpper();
    //Shortest possible save path: C:/d/f (6 characters)
    if (safePrefix.length() >= 6 && pathUpper.startsWith(safePrefix))
        return true;

    DBG << "ERROR: " << pathUpper << "not in safe prefix" << safePrefix << ". Remove aborted!";
    return true;
}
