#include <QDir>
#include "szip.h"

#ifdef Q_OS_LINUX
    const QString Szip::SZIP_EXECUTABLE = "7za";
#else
    const QString Szip::SZIP_EXECUTABLE = "bin\\7za.exe";
#endif

Szip::Szip()
{

}

bool Szip::compress(const QString& dir, const QString& archivePath)
{
    //Create 7zip archive. r = recurse, y = assume yes to everything
    //-t7z   7z archive
    //-m0=lzma2 lzma2 method
    //-mx=9  level of compression = 9 (Ultra)
    //-mfb=64 number of fast bytes for LZMA = 64
    //-md=32m dictionary size = 32 megabytes
    //-ms=on solid archive = on
    //-mx=9 compression level
    return console_.runCmd(SZIP_EXECUTABLE + " a -r -y -t7z -m0=lzma2 -mx=9 -mfb=64 -md=32m -ms=on "
                      + QDir::toNativeSeparators(archivePath) + " "
                      + QDir::toNativeSeparators(dir));
}
