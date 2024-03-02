#include <QChar>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include "ahasher.h"

const unsigned AHasher::BASE = 36;
const unsigned AHasher::MAX_VALUE = 1679615; // ZZZZ

QString AHasher::hash(QList<QFileInfo> files)
{
    qint64 size = 0;
    for (const QFileInfo& fi : files)
        size += fi.size();

    qint64 modulated = size % MAX_VALUE; //Ensures the value is within 4 chars.
    QString rVal = baseEncode(modulated);
    for (int padding = 4 - rVal.size(); padding > 0; --padding)
        rVal.prepend("0");

    return rVal;
}

QString AHasher::hash(const QString& dirPath)
{
    QList<QFileInfo> files;
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo fi = it.next();
        files.append(fi);
    }
    return hash(files);
}

QString AHasher::baseEncode(qint64 input)
{
    static char const base36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    QString output;

    qint64 div = input / BASE;
    char re = base36[input % BASE]; //Compiler will optimize.

    if (div != 0)
        output += baseEncode(div);

    return output.append(re);
}

