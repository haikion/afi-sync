#include "fileutils.h"
#include "settingsmodel.h"
#include "repository.h"
#include "installer.h"
#include "debug.h"

void Installer::install(const Mod* mod)
{
    QString modPath = SettingsModel::modDownloadPath() + "/" + mod->name();
    //TeamSpeak 3 plugins
    QDir tsDir(modPath + "/teamspeak 3 client");
    install(tsDir, SettingsModel::teamSpeak3Path());

    QDir(SettingsModel::arma3Path()).mkdir("userconfig");
    //User config
    QDir modUserConfig(modPath + "/userconfig");
    install(modUserConfig, QDir(SettingsModel::arma3Path() + "/userconfig"));

    //User config 2
    QDir modUserConfig2(modPath + "/store/userconfig");
    install(modUserConfig2, QDir(SettingsModel::arma3Path() + "/userconfig"));
}

void Installer::install(const QDir& src, const QDir& dst)
{
    if (!src.exists() || !dst.exists())
    {
        DBG << "Nothing to install src =" << src.absolutePath();
        return;
    }
    DBG << "Installing" << src.absolutePath()
             << "to" << dst.absolutePath();
    FileUtils::copy(src.absolutePath(), dst.absolutePath());
}
