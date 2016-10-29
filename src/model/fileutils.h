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
    static bool writeFile(const QByteArray& data, const QString& path);
    //Case insensitive removal
    static bool rmCi(QString path);
    //Safety functions. These assure that the file being handeled is in safe subpath.
    //Prevents nasty programming errors from deleting important files.
    static bool safeRemove(QFile& file);
    static bool safeRemove(const QString& filePath);
    static bool safeRemoveRecursively(QDir& dir);
    static bool safeRename(const QString& srcPath, const QString& dstPath);

private:
    static bool pathIsSafe(const QString& path);
    static QString casedPath(const QString& path);
};

#endif // FILEUTILS_H
