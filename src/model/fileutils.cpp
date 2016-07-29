#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QStringList>
#include <QDirIterator>
#include "debug.h"
#include "fileutils.h"

//https://qt.gitorious.org/qt-creator/qt-creator/source/1a37da73abb60ad06b7e33983ca51b266be5910e:src/app/main.cpp#L13-189
// taken from utils/fileutils.cpp. We can not use utils here since that depends app_version.h.
bool FileUtils::copy(const QString& srcPath, const QString& dstPath)
{
    DBG << dstPath;
    QFileInfo srcFi(srcPath);
    //Directory
    if (srcFi.isDir())
    {
        QDir dstDir(dstPath);
        if (!dstDir.exists())
        {
            //Rename root directory
            dstDir.cdUp();
        }

        QString dirName = QFileInfo(dstPath).fileName();
        dstDir.mkpath(dirName);
        if (!QFileInfo(dstPath).exists())
        {
            DBG << "ERROR: Failed to create directory" << dirName;
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
        dstFile.remove();
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

bool FileUtils::rmCi(QString path)
{
    path.replace("\\", "/");
    if (!path.startsWith("/"))
    {
        DBG << "ERROR: Only absolute paths are supported";
    }
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
    return QFile(casedPath).remove();
}
