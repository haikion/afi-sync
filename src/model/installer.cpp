#include "fileutils.h"
#include "settingsmodel.h"
#include "pathfinder.h"
#include "repository.h"
#include "installer.h"
#include "debug.h"

void Installer::install(const Mod* mod)
{
    QString modPath = SettingsModel::modDownloadPath() + "/" + mod->name();
    //TeamSpeak 3 plugins. Install to all possible plugin locations.
    QDir tsDir(modPath + "/teamspeak 3 client");
    install(tsDir, SettingsModel::teamSpeak3Path()); //TeamSpeak 3 path
    install(tsDir, PathFinder::teamspeak3AppDataPath()); //AppData plugins

    QDir(SettingsModel::arma3Path()).mkdir("userconfig");
    //User config
    QDir modUserConfig(modPath + "/userconfig");
    install(modUserConfig, SettingsModel::arma3Path() + "/userconfig");

    //User config 2
    QDir modUserConfig2(modPath + "/store/userconfig");
    install(modUserConfig2, SettingsModel::arma3Path() + "/userconfig");
}

void Installer::install(const QDir& src, const QDir& dst)
{
    if (!src.exists())
    {
        DBG << "Nothing to install from" << src.absolutePath();
        return;
    }
    if (!dst.exists())
    {
        DBG << "Warning: Destination directory:" << dst.absolutePath() << "does not exist.";
        return;
    }
    DBG << "Installing" << src.absolutePath()
             << "to" << dst.absolutePath();
    FileUtils::copy(src.absolutePath(), dst.absolutePath());
}
