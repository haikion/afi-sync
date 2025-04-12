#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QDir>
#include <QString>
#include <QStringList>

namespace FileUtils
{
    bool copy(const QString& srcPath, const QString& dstPath);
    bool move(const QString& srcPath, const QString& dstPath);
    [[nodiscard]] qint64 dirSize(const QString& path);
    [[nodiscard]] QByteArray readFile(const QString& path);
    bool writeFile(const QByteArray& data, const QString& path);
    //Case insensitive removal
    bool rmCi(const QString& path);
    //Safety functions. These assure that the file being handeled is in safe subpath.
    //Prevents nasty programming errors from deleting important files.
    bool safeRemove(QFile& file);
    bool safeRemove(const QString& filePath);
    bool safeRemoveRecursively(QDir& dir);
    bool safeRemoveRecursively(const QString& path);
    bool safeRename(const QString& srcPath, const QString& dstPath);
    void safeRemoveAll(const QStringList& paths);
    // Removes all empty directories from path
    void safeRemoveEmptyDirs(const QString& path);
    //For testability
    void appendSafePath(const QString& path);
    [[nodiscard]] QString casedPath(const QString& path);
    [[nodiscard]] bool filesIdentical(const QString& path1, const QString& path2);
    [[nodiscard]] QStringList getSafeSubPaths();
};

#endif // FILEUTILS_H
