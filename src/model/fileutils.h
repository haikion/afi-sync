#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QString>

class FileUtils
{
public:
    static bool copy(const QString& srcPath, const QString& dstPath);
};

#endif // FILEUTILS_H
