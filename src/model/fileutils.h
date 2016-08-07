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
};

#endif // FILEUTILS_H
