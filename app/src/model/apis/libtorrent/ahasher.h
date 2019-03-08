/*
 * Calculates AFISync hash from given files.
 */
#ifndef AHASHER_H
#define AHASHER_H

#include <QFileInfo>
#include <QList>

class AHasher
{
public:
    //Returns combined size or overflowed value if MAX_VALUE is exceeded
    //in base36 format.
    static QString hash(QList<QFileInfo> files);
    static QString hash(const QString& dirPath);
    static QString baseEncode(qint64 input);

private:
    static const unsigned BASE;
    static const unsigned MAX_VALUE;
};

#endif // AHASHER_H
