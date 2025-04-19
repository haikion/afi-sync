#include <algorithm>

#include <QSettings>
#include <QStringLiteral>

#include "afisynclogger.h"
#include "fileutils.h"
#include "installer.h"
#include "paths.h"
#include "settingsmodel.h"
#include "szip.h"

using namespace Qt::StringLiterals;

namespace {
bool sortFiles(const QString& file1, const QString& file2) {
    // Move files with ".dll" postfix to the end
    if (file1.endsWith(".dll"_L1) && !file2.endsWith(".dll"_L1))
    {
        return false;
    }
    if (!file1.endsWith(".dll"_L1) && file2.endsWith(".dll"_L1))
    {
        return true;
    }
    // Sort other files alphabetically
    return file1 < file2;
}

QStringList listFilesInRelativeForm(const QDir& directory) {
    QStringList filesList;

    // Get the list of files in the directory
    const QStringList currentFiles = directory.entryList(QDir::Files);
    filesList.reserve(currentFiles.size());
    for (const QString &file : currentFiles) {
        filesList.append(directory.relativeFilePath(file));
    }

    // Get the list of directories in the directory
    const QStringList dirsList = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    // Iterate over each directory and recursively get the list of files
    for (const QString &dir : dirsList) {
        QDir subDir = directory;
        subDir.cd(dir);
        QStringList subFilesList = listFilesInRelativeForm(subDir);
        for (QString &subFile : subFilesList) {
            // Prepend the relative directory path to the file name
            subFile.prepend(dir + "/");
        }
        // Append the files from subdirectories to the main list
        filesList.append(subFilesList);
    }

    return filesList;
}
}

void Installer::install(Mod* mod)
{
    SettingsModel& settings = settings.instance();
    QString modPath = settings.modDownloadPath() + "/" + mod->name();
    //TeamSpeak 3 plugins. Install to all possible plugin locations.
    auto path = FileUtils::casedPath(modPath + "/teamspeak 3 client");
    if (!path.isEmpty())
    {
        QDir tsDir(path);
        auto addonsPath = settings.teamSpeak3Path() + "/config/addons.ini";
        if (install(tsDir, settings.teamSpeak3Path() + "/config")
            && QFile::exists(addonsPath))
        {
            QDir pluginsDir(path + "/plugins");
            QStringList list = listFilesInRelativeForm(pluginsDir);
            std::sort(list.begin(), list.end(), sortFiles);
            QSettings settings(addonsPath, QSettings::IniFormat);
            settings.beginGroup("plugin");
            settings.setValue("Task Force Arrowhead Radio v1/files", list);
        }
        install(tsDir, Paths::teamspeak3AppDataPath()); //AppData plugins
    }

    // Install TeamSpeak 3 plugin files (*.ts3_plugin)
    installTeamSpeak3Plugin(modPath, settings.teamSpeak3Path(), Paths::teamspeak3AppDataPath());

    QDir(settings.arma3Path()).mkdir(u"userconfig"_s);
    //User config
    QDir modUserConfig(modPath + "/userconfig");
    install(modUserConfig, settings.arma3Path() + "/userconfig");

    //User config 2
    QDir modUserConfig2(modPath + "/store/userconfig");
    install(modUserConfig2, settings.arma3Path() + "/userconfig");
}

bool Installer::install(const QDir& src, const QDir& dst)
{
    if (!src.exists())
    {
        LOG << "Nothing to install from " << src.absolutePath();
        return false;
    }
    if (!dst.exists())
    {
        LOG_WARNING << "Destination directory: " << dst.absolutePath() << " does not exist.";
        return false;
    }
    LOG << "Installing " << src.absolutePath() << " to " << dst.absolutePath();
    FileUtils::copy(src.absolutePath(), dst.absolutePath());
    return true;
}

bool Installer::installTeamSpeak3Plugin(const QString& modPath, const QString& ts3Path, const QString& ts3AppDataPath)
{
    // Check for TeamSpeak directory
    QDir teamSpeakDir(modPath + "/teamspeak");
    if (!teamSpeakDir.exists())
    {
        LOG << "No TeamSpeak directory found in " << modPath;
        return false;
    }

    QStringList filters;
    filters << "*.ts3_plugin";
    teamSpeakDir.setNameFilters(filters);
    QStringList pluginFiles = teamSpeakDir.entryList(QDir::Files);
    if (pluginFiles.isEmpty())
    {
        LOG << "No TS3 plugin files found in " << teamSpeakDir.absolutePath();
        return false;
    }
    QString pluginFile = pluginFiles.first();
    QString pluginFilePath = teamSpeakDir.absoluteFilePath(pluginFile);
    LOG << "Found TS3 plugin file: " << pluginFilePath;

    QDir tempDir(QDir::tempPath() + "/ts3plugin_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    if (!tempDir.exists())
    {
        QDir().mkpath(tempDir.absolutePath());
    }
    
    // Extract the plugin file using 7za
    // TODO: Do this asynchronously
    Szip szip;
    if (!szip.extract(pluginFilePath, tempDir.absolutePath()))
    {
        LOG_WARNING << "Failed to extract TS3 plugin file: " << pluginFilePath;
        return false;
    }
    LOG << "Extracted TS3 plugin to: " << tempDir.absolutePath();
    
    // Install the extracted plugin to TeamSpeak paths
    bool mainInstallSuccess = install(tempDir, ts3Path + "/config");
    bool appDataInstallSuccess = install(tempDir, ts3AppDataPath);
    
    // Update addons.ini if it exists
    if (mainInstallSuccess)
    {
        auto addonsPath = ts3Path + "/config/addons.ini";
        if (QFile::exists(addonsPath))
        {
            // Read package name
            QSettings pkgSettings(tempDir.absolutePath() + "/package.ini", QSettings::IniFormat);
            const QString pluginName = pkgSettings.value("Name").toString();
            if (pluginName.isEmpty())
            {
                LOG_WARNING << "Unable to read plugin name";
                return false;
            }
            if (!tempDir.cd("plugins"))
            {
                LOG_WARNING << "Unable to find plugins folder";
                return false;
            }
            QStringList list = listFilesInRelativeForm(tempDir);
            std::sort(list.begin(), list.end(), sortFiles);
            QSettings settings(addonsPath, QSettings::IniFormat);
            settings.beginGroup("plugin");
            settings.setValue(pluginName + "/files", list);
        }
    }
    return mainInstallSuccess || appDataInstallSuccess;
}
