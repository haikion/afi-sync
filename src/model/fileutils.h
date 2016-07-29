#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>

class FileUtils
{
public:
    static bool copy(const QString& srcPath, const QString& dstPath);
    //Case insensitive removal
    static bool rmCi(QString path);
};

#endif // FILEUTILS_H
