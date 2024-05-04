#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QDir>
#include <QString>
#include <QStringList>

class FileUtils
{
public:
    static bool copy(const QString& srcPath, const QString& dstPath);
    static bool move(const QString& srcPath, const QString& dstPath);
    [[nodiscard]] static qint64 dirSize(const QString& path);
    [[nodiscard]] static QByteArray readFile(const QString& path);
    static bool writeFile(const QByteArray& data, const QString& path);
    //Case insensitive removal
    static bool rmCi(const QString& path);
    //Safety functions. These assure that the file being handeled is in safe subpath.
    //Prevents nasty programming errors from deleting important files.
    static bool safeRemove(QFile& file);
    static bool safeRemove(const QString& filePath);
    static bool safeRemoveRecursively(QDir& dir);
    static bool safeRemoveRecursively(const QString& path);
    static bool safeRename(const QString& srcPath, const QString& dstPath);
    // Removes all empty directories from path
    static void safeRemoveEmptyDirs(const QString& path);
    //For testability
    static void appendSafePath(const QString& path);
    [[nodiscard]] static QString casedPath(const QString& path);
    [[nodiscard]] static bool filesIdentical(const QString& path1, const QString& path2);

private:
    static QStringList safeSubpaths_;

    static bool pathIsSafe(const QString& path);
};

#endif // FILEUTILS_H
