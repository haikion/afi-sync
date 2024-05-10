#include "ahasher.h"

#include <QChar>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QStringLiteral>

#include "model/afisynclogger.h"

using namespace Qt::StringLiterals;

namespace {
    const unsigned BASE = 36;
    const unsigned MAX_VALUE = 1679615; // ZZZZ

    QString baseEncode(qint64 input)
    {
        static char const base36[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        QString output;

        qint64 div = input / BASE;
        char re = base36[input % BASE]; //Compiler will optimize.

        if (div != 0)
        {
            output += baseEncode(div);
        }

        return output.append(re);
    }

    QString hashFiles(const QList<QFileInfo>& files)
    {
        QCryptographicHash hash{QCryptographicHash::Md5};
        for (const QFileInfo& fi : files) {
            hash.addData(QString::number(fi.size()).toUtf8());
            hash.addData(fi.fileName().toUtf8());
            if (fi.isFile() && fi.fileName().endsWith(".bisign"_L1)) {
                QFile file(fi.filePath());
                if (file.open(QIODevice::ReadOnly)) {
                    hash.addData(file.readAll());
                } else {
                    LOG_WARNING << "Unable to open file" << fi.filePath();
                }
            }
        }
        return hash.result().toHex();
    }

    QString legacyHashFiles(const QList<QFileInfo>& files)
    {
        qint64 size = 0;
        for (const QFileInfo& fi : files)
        {
            size += fi.size();
        }

        qint64 modulated = size % MAX_VALUE; //Ensures the value is within 4 chars.
        QString rVal = baseEncode(modulated);
        for (auto padding = 4 - rVal.size(); padding > 0; --padding)
        {
            rVal.prepend('0');
        }

        return rVal;
    }

}

QString AHasher::hash(const QString& dirPath)
{
    QList<QFileInfo> files;
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo fi = it.nextFileInfo();
        files.append(fi);
    }
    return hashFiles(files);
}

QString AHasher::legacyHash(const QString& dirPath)
{
    QList<QFileInfo> files;
    QDirIterator it(dirPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo fi = it.nextFileInfo();
        files.append(fi);
    }
    return legacyHashFiles(files);
}
