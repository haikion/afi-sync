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
    static QByteArray readFile(const QString& path);
    //Case insensitive removal
    static bool rmCi(QString path);
    static bool safeRemove(QFile& file);
    static bool safeRemove(const QString& filePath);
    static bool safeRemoveRecursively(QDir& dir);
    static bool safeRename(const QString& srcPath, const QString& dstPath);

private:
    static bool pathIsSafe(const QString& path);
    static QString casedPath(const QString& path);
};

#endif // FILEUTILS_H
