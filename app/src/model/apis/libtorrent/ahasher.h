/*
 * Calculates AFISync hash from given files.
 */
#ifndef AHASHER_H
#define AHASHER_H

#include <QString>

namespace AHasher
{
    QString hash(const QString& dirPath);
    QString legacyHash(const QString& dirPath);
};

#endif // AHASHER_H
