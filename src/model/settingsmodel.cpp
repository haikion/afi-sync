#include "debug.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include "pathfinder.h"
#include "settingsmodel.h"
#include "global.h"

//TODO: Combine set and enabled

QSettings* SettingsModel::settings_ = nullptr;

SettingsModel::SettingsModel(QObject* parent):
    QObject(parent)
{
    createSettings();
}

void SettingsModel::createSettings()
{
    DBG << "Creating settings object.";
    QCoreApplication::setOrganizationName("AFISync");
    QCoreApplication::setOrganizationDomain("armafinland.fi");
    QCoreApplication::setApplicationName("AFISync");

    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, SettingsModel::settingsPath());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    settings_ = new QSettings();
}

QSettings* SettingsModel::settings()
{
    if (!settings_)
        createSettings();

    return settings_;
}

//Saves only valid dirs
bool SettingsModel::saveDir(const QString& key, const QString& path)
{
    QFileInfo dir(path);
    QString originalPath = settings()->value(key).toString();
    if (originalPath == path || !dir.isDir() || !dir.isWritable())
    {
        return false;
    }
    settings()->setValue(key, path);
    return true;
}

//Sets default value for performance reasons.
QString SettingsModel::setting(const QString& key, const QString& defaultValue)
{
    QString set = settings()->value(key).toString();
    if (set == QString())
    {
        DBG << "Setting default value" << defaultValue << "for" << key;
        settings()->setValue(key, defaultValue);
        set = defaultValue;
    }
    return set;
}

QString SettingsModel::arma3Path()
{
   return setting("arma3Dir", PathFinder::arma3Path());
}

void SettingsModel::setArma3Path(const QString& path)
{
    saveDir("arma3Dir", path);
}

void SettingsModel::resetArma3Path()
{
    settings()->setValue("arma3Dir", PathFinder::arma3Path());
}

QString SettingsModel::teamSpeak3Path()
{
   return setting("teamSpeak3Path", PathFinder::teamspeak3Path());
}

void SettingsModel::setTeamSpeak3Path(const QString& path)
{
    saveDir("teamSpeak3Path", path);
}

void SettingsModel::resetTeamSpeak3Path()
{
    settings()->setValue("teamSpeak3Path", PathFinder::teamspeak3Path());
}

QString SettingsModel::steamPath()
{
    DBG;
    return settings()->value("steamPath", PathFinder::steamPath()).toString();
}

void SettingsModel::setSteamPath(const QString& path)
{
    settings()->setValue("steamPath", path);
}

void SettingsModel::resetSteamPath()
{
    DBG;
    settings()->setValue("steamPath", PathFinder::steamPath());
}


QString SettingsModel::maxUpload()
{
    QString rVal = settings()->value("maxUpload", "").toString();
    return rVal;
}

void SettingsModel::setMaxDownload(const QString& value)
{
    settings()->setValue("maxDownload", value);
}

QString SettingsModel::maxDownload()
{
    return settings()->value("maxDownload", "").toString();
}

void SettingsModel::setMaxDownloadEnabled(bool enabled)
{
    settings()->setValue("maxDownloadEnabled", enabled);
    //Activate or disable limit
    Global::sync->setMaxDownload(enabled ? maxDownload().toUInt() : 0);
}

bool SettingsModel::maxDownloadEnabled()
{
    return settings()->value("maxDownloadEnabled", false).toBool();
}

void SettingsModel::setMaxUploadEnabled(bool enabled)
{
    settings()->setValue("maxUploadEnabled", enabled);
    Global::sync->setMaxUpload(enabled ? maxUpload().toUInt() : 0);
}

bool SettingsModel::maxUploadEnabled()
{
    return settings()->value("maxUploadEnabled", false).toBool();
}

void SettingsModel::setInstallDate(const QString& repoName, const unsigned& value)
{
    settings()->setValue("installDate" + repoName, value);
}

unsigned SettingsModel::installDate(const QString& repoName)
{
    return settings()->value("installDate" + repoName).toInt();
}

void SettingsModel::setPort(const QString& port)
{
    settings()->setValue("port", port);
}

void SettingsModel::setPortEnabled(bool enabled)
{
    settings()->setValue("portEnabled", enabled);
    Global::sync->setPort(enabled ? port().toInt() : Constants::DEFAULT_PORT.toInt());
}

bool SettingsModel::portEnabled()
{
    return settings()->value("portEnabled", false).toBool();
}

QString SettingsModel::port()
{
    return settings()->value("port", Constants::DEFAULT_PORT).toString();
}

QString SettingsModel::settingsPath()
{
    return QCoreApplication::applicationDirPath() + "/settings";
}

void SettingsModel::setTicked(const QString& modName, QString repoName, bool value)
{
    QString repoStr = repoName.replace("/| ","_");
    QString key = modName.isEmpty() ? repoName + "/checked" : modName + "/" + repoStr + "ticked";
    settings()->setValue(key, value);
}

bool SettingsModel::ticked(const QString& modName, QString repoName)
{

    QString repoStr = repoName.replace("/| ","_");
    QString key = modName.isEmpty() ? repoName + "/checked" : modName + "/" + repoStr + "ticked";
    return settings()->value(key, false).toBool();
}

void SettingsModel::setProcess(const QString& name, bool value)
{
    settings()->setValue(name + "/process", value);
}

bool SettingsModel::process(const QString& name)
{
    return settings()->value(name + "/process", true).toBool();
}

QString SettingsModel::syncSettingsPath()
{
    return settingsPath() + "/sync";
}

void SettingsModel::setMaxUpload(const QString& value)
{
    settings()->setValue("maxUpload", value);
}

bool SettingsModel::battlEyeEnabled()
{
    return settings()->value("battlEyeEnabled", true).toBool();
}

void SettingsModel::setDeltaPatchingEnabled(bool enabled)
{
    settings()->setValue("deltaPatchingEnabled", enabled);
    if (Global::sync)
    {
        if (enabled)
            Global::sync->enableDeltaUpdates();
        else
            Global::sync->disableDeltaUpdates();
    }
}

bool SettingsModel::deltaPatchingEnabled()
{
    return settings()->value("deltaPatchingEnabled", false).toBool();
}

void SettingsModel::setBattlEyeEnabled(bool enabled)
{
    settings()->setValue("battlEyeEnabled", enabled);
}

QString SettingsModel::modDownloadPath()
{
    return QDir::fromNativeSeparators(settings()->value("modDownloadPath", PathFinder::arma3Path()).toString());
}

void SettingsModel::setModDownloadPath(QString path)
{
    DBG << "path =" << path;
    path = QDir::fromNativeSeparators(path);
    if (!saveDir("modDownloadPath", path))
    {
        DBG << "Warning: failed to set mod download path. modDownloadPath() ="
            << modDownloadPath() << " path =" << path;
        return;
    }
    settings()->setValue("modDownloadPath", path);
    if (Global::model != nullptr) //TODO: Consider removing, seems too defensive.
        Global::model->reset();
}

void SettingsModel::resetModDownloadPath()
{
    setModDownloadPath(PathFinder::arma3Path());
}

QString SettingsModel::launchParameters()
{
    return settings()->value("launchParameters", "-noSplash -world=empty -skipIntro").toString();
}

void SettingsModel::setLaunchParameters(const QString& parameters)
{
    settings()->setValue("launchParameters", parameters);
}
