#ifndef SZIP_H
#define SZIP_H

#include "console.h"

class Szip
{
public:
    Szip();
    bool compress(const QString& dir, const QString& archivePath);
private:
    static const QString SZIP_EXECUTABLE;

    Console console_;
};

#endif // SZIP_H
