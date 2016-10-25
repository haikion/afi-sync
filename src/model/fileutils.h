#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QDir>
#include <QString>

class FileUtils
{
public:
    static bool copy(const QString& srcPath, const QString& dstPath);
    static bool move(const QString& srcPath, const QString& dstPath);
    static qint64 dirSize(const QString& path);
    //Case insensitive removal
    static bool rmCi(QString path);
    static bool safeRemove(const QFile& file);
    static bool safeRemoveRecursively(const QDir& dir);
    static bool safeRename(const QString& srcPath, const QString& dstPath);

private:
    static bool pathIsSafe(const QString& path);
};

#endif // FILEUTILS_H
