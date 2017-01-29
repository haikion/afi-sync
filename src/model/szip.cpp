#include <QDir>
#include "szip.h"

#ifdef Q_OS_LINUX
    const QString Szip::SZIP_EXECUTABLE = "7za";
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

bool Szip::compress(const QString& dir, const QString& archivePath) const
{
    return console_.runCmd(COMMAND + " " + QDir::toNativeSeparators(archivePath) + " "
                      + QDir::toNativeSeparators(dir));
}

QProcess* Szip::compressAsync(const QString& dir, const QString& archivePath)
{
    return console_.runCmdAsync(COMMAND + " " + QDir::toNativeSeparators(archivePath) + " "
                      + QDir::toNativeSeparators(dir));
}
