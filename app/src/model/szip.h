#ifndef SZIP_H
#define SZIP_H

#include "console.h"

class Szip
{
public:
    Szip() = default;

    bool compress(const QString& dir, const QString& archivePath) const;
    QProcess* compressAsync(const QString& dir, const QString& archivePath);

private:
    static const QString SZIP_EXECUTABLE;
    static const QString COMMAND;

    Console console_;
};

#endif // SZIP_H
