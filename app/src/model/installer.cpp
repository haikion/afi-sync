#include <algorithm>

#include <QSettings>
#include <QStringLiteral>

#include "afisynclogger.h"
#include "fileutils.h"
#include "installer.h"
#include "pathfinder.h"
#include "settingsmodel.h"

using namespace Qt::StringLiterals;

namespace {
bool sortFiles(const QString &file1, const QString &file2) {
    // Move files with ".dll" postfix to the end
    if (file1.endsWith(".dll"_L1) && !file2.endsWith(".dll"_L1))
        return false;
    if (!file1.endsWith(".dll"_L1) && file2.endsWith(".dll"_L1))
        return true;
    // Sort other files alphabetically
    return file1 < file2;
}

QStringList listFilesInRelativeForm(const QDir &directory) {
    QStringList filesList;

    // Get the list of files in the directory
    QStringList currentFiles = directory.entryList(QDir::Files);
    filesList.reserve(currentFiles.size());
    for (const QString &file : currentFiles) {
        filesList.append(directory.relativeFilePath(file));
    }

    // Get the list of directories in the directory
    QStringList dirsList = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

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
    QString modPath = SettingsModel::modDownloadPath() + "/" + mod->name();
    //TeamSpeak 3 plugins. Install to all possible plugin locations.
    auto path = FileUtils::casedPath(modPath + "/teamspeak 3 client");
    if (!path.isEmpty())
    {
        QDir tsDir(path);
        auto addonsPath = SettingsModel::teamSpeak3Path() + "/config/addons.ini";
        if (install(tsDir, SettingsModel::teamSpeak3Path() + "/config")
            && QFile::exists(addonsPath)) {
            QDir pluginsDir(path + "/plugins");
            QStringList list = listFilesInRelativeForm(pluginsDir);
            std::sort(list.begin(), list.end(), sortFiles);
            QSettings settings(addonsPath, QSettings::IniFormat);
            settings.beginGroup("plugin");
            settings.setValue("Task Force Arrowhead Radio v1/files", list);
        }
        install(tsDir, PathFinder::teamspeak3AppDataPath()); //AppData plugins
    }

    QDir(SettingsModel::arma3Path()).mkdir(u"userconfig"_s);
    //User config
    QDir modUserConfig(modPath + "/userconfig");
    install(modUserConfig, SettingsModel::arma3Path() + "/userconfig");

    //User config 2
    QDir modUserConfig2(modPath + "/store/userconfig");
    install(modUserConfig2, SettingsModel::arma3Path() + "/userconfig");
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
