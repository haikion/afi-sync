#include "debug.h"
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
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
    //QDir dir(path);
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
    return QDir::toNativeSeparators(path);
}

QString PathFinder::steamPath()
{
    QString path = readRegPath("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam", "InstallPath");
    if (path == QCoreApplication::applicationDirPath())
    {
        path = readRegPath("HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam", "InstallLocation");
    }
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
    return QDir::toNativeSeparators(path);
}
