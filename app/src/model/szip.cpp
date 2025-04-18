#include <QDir>
#include <QStringLiteral>
#include "szip.h"

using namespace Qt::StringLiterals;

#ifdef Q_OS_LINUX
const QString Szip::SZIP_EXECUTABLE = u"7za"_s;
#else
    const QString Szip::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

//Create 7zip archive. r = recurse, y = assume yes to everything
//-t7z   7z archive
//-m0=lzma2 lzma2 method
//-mx=9  level of compression = 9 (Ultra)
//-mfb=64 number of fast bytes for LZMA = 64
//-md=32m dictionary size = 32 megabytes
//-ms=on solid archive = on
//-mx=9 compression level
//-sdel delete files after compression
const QString Szip::COMMAND = SZIP_EXECUTABLE  + " a -r -y -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on -sdel";

bool Szip::compress(const QString& dir, const QString& archivePath)
{
    return console_.runCmd(COMMAND + " " + QDir::toNativeSeparators(archivePath) + " "
                      + QDir::toNativeSeparators(dir));
}

bool Szip::extract(const QString& path, const QString& outputDir)
{
    const auto pathNative = QDir::toNativeSeparators(path);
    const auto outputDirNative = QDir::toNativeSeparators(outputDir);
    QString extractCommand = QString("%1 x \"%2\" -o\"%3\" -y").arg(SZIP_EXECUTABLE, pathNative, outputDirNative);
    return console_.runCmd(extractCommand);
}

QProcess* Szip::compressAsync(const QString& dir, const QString& archivePath)
{
    return console_.runCmdAsync(COMMAND + " " + QDir::toNativeSeparators(archivePath) + " "
                      + QDir::toNativeSeparators(dir));
}
