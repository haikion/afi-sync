#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include "afisynclogger.h"
#include "global.h"
#include "pathfinder.h"

QString PathFinder::arma3Path()
{
    //Windows 7
    //HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Bohemia Interactive\arma 3
    QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Bohemia Interactive\\arma 3", QSettings::NativeFormat);
    QString path = settings.value( "main", QCoreApplication::applicationDirPath()).toString();
    if (path == QCoreApplication::applicationDirPath())
    {
        //Windows 8.1
        //HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 107410
        QSettings settings2("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 107410", QSettings::NativeFormat);
        path = settings2.value( "InstallLocation", QCoreApplication::applicationDirPath()).toString();
    }
    checkPath(path, "arma 3");
    return QDir::toNativeSeparators(path);
}

QString PathFinder::teamspeak3Path()
{
    //Windows 8.1
    QSettings settings2("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\TeamSpeak 3 Client", QSettings::NativeFormat);
    QString path = settings2.value( "InstallLocation", QCoreApplication::applicationDirPath()).toString();
    if (path == QCoreApplication::applicationDirPath())
    {
        //Windows 7
        QSettings settings("HKEY_CLASSES_ROOT\\ts3file\\shell\\open\\command", QSettings::NativeFormat);
        path = settings.value("Default", QCoreApplication::applicationDirPath()).toString();
        path.remove(" \"%1\"");
        path.replace("\\ts3client_win64.exe", "");
        path.remove("\"");
    }
    checkPath(path, "TeamSpeak 3");
    return QDir::toNativeSeparators(path);
}

QString PathFinder::teamspeak3AppDataPath()
{
    QDir dir =  QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.cd("../../TS3Client");

    return QDir::toNativeSeparators(dir.absolutePath()); //TODO: Consider discarding this as toNative conversion should be done only on UI-layer.
}


QString PathFinder::steamPath()
{
    QString path = readRegPath("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam", "InstallPath");
    if (path == QCoreApplication::applicationDirPath())
    {
        path = readRegPath("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam", "InstallLocation");
    }
    checkPath(path, "Steam");
    return path;
}

QString PathFinder::readRegPath(const QString& path, const QString& key)
{
    QSettings settings(path, QSettings::NativeFormat);
    QString rPath = settings.value(key, QCoreApplication::applicationDirPath()).toString();
    return QDir::toNativeSeparators(rPath);
}

QString PathFinder::arma3MyDocuments()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Arma 3";
    checkPath(path, "My Documents\\Arma3");
    return QDir::toNativeSeparators(path);
}

//Prints error if path is default which means it wasn't found from regs.
void PathFinder::checkPath(const QString& path, const QString& name)
{
    #ifndef Q_OS_LINUX
    if (path == QCoreApplication::applicationDirPath()
            && !Global::guiless)
    {
        LOG_ERROR << "Unable to find path for " << name
            << " Using default: " << path;
    }
    #endif
}
