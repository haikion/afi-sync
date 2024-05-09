#include <QCoreApplication>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>

#include "paths.h"
#ifdef Q_OS_WIN
#include "afisynclogger.h"
#include "global.h"
#endif

using namespace Qt::StringLiterals;

namespace {
    QString readRegPath(const QString& path, const QString& key)
    {
        QSettings settings(path, QSettings::NativeFormat);
        QString rPath = settings.value(key, QCoreApplication::applicationDirPath()).toString();
        return QDir::toNativeSeparators(rPath);
    }

    //Prints error if path is default which means it wasn't found from regs.
    #ifdef Q_OS_WIN
    void checkPath(const QString& path, const QString& name)
    {
        if (path == QCoreApplication::applicationDirPath()
                && !Global::guiless)
        {
            LOG_WARNING << "Unable to find path for " << name
                << " Using default: " << path;
        }
    }
    #else
    void checkPath(const QString&, const QString&) {}
    #endif
}

QString Paths::arma3Path()
{
    //Windows 7
    //HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Bohemia Interactive\arma 3
    QSettings
        settings(QStringLiteral(
                     "HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Bohemia Interactive\\arma 3"),
                 QSettings::NativeFormat);
    QString path = settings.value( "main", QCoreApplication::applicationDirPath()).toString();
    if (path == QCoreApplication::applicationDirPath())
    {
        //Windows 8.1
        //HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 107410
        QSettings settings2(u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 107410"_s, QSettings::NativeFormat);
        path = settings2.value( "InstallLocation", QCoreApplication::applicationDirPath()).toString();
    }
    checkPath(path, u"arma 3"_s);
    return QDir::toNativeSeparators(path);
}

QString Paths::teamspeak3Path()
{
    // Windows 10
    QSettings settingsCommand("HKEY_CURRENT_USER\\SOFTWARE\\Classes\\ts3addon\\shell\\open\\command", QSettings::NativeFormat);
    QString cmdPath = settingsCommand.value("Default"_L1, {}).toString();
    if (cmdPath.endsWith(R"(\package_inst.exe" "%1")"_L1))
    {
        cmdPath.remove(R"(\package_inst.exe" "%1")"_L1);
        if (cmdPath.startsWith('\"'))
        {
            cmdPath.remove('\"');
            return QDir::toNativeSeparators(cmdPath);
        }
    }

    //Windows 8.1
    QSettings settings2(u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\TeamSpeak 3 Client"_s, QSettings::NativeFormat);
    QString path = settings2.value( "InstallLocation", QCoreApplication::applicationDirPath()).toString();
    if (path == QCoreApplication::applicationDirPath())
    {
        //Windows 7
        QSettings settings(u"HKEY_CLASSES_ROOT\\ts3file\\shell\\open\\command"_s,
                           QSettings::NativeFormat);
        path = settings.value("Default", QCoreApplication::applicationDirPath()).toString();
        path.remove(" \"%1\""_L1);
        path.replace("\\ts3client_win64.exe"_L1, ""_L1);
        path.remove('\"');
    }
    checkPath(path, u"TeamSpeak 3"_s);
    return QDir::toNativeSeparators(path);
}

QString Paths::teamspeak3AppDataPath()
{
    QDir dir =  QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.cd(u"../../TS3Client"_s);

    return QDir::toNativeSeparators(dir.absolutePath()); //TODO: Consider discarding this as toNative conversion should be done only on UI-layer.
}

QString Paths::steamPath()
{
    QString path = readRegPath(u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam"_s,
                               u"InstallPath"_s);
    if (path == QCoreApplication::applicationDirPath())
    {
        path = readRegPath(
            u"HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam"_s,
            u"InstallLocation"_s);
    }
    checkPath(path, u"Steam"_s);
    return path;
}
