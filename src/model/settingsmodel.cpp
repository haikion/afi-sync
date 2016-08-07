#include "debug.h"
#include <QFileInfo>
#include <QCoreApplication>
#include "pathfinder.h"
#include "settingsmodel.h"
#include "global.h"

QSettings* SettingsModel::settings_ = nullptr;

SettingsModel::SettingsModel(QObject* parent):
    QObject(parent)
{
    settings();
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

QSettings* SettingsModel::settings()
{
    if (settings_ == nullptr)
    {
        DBG << "Creating settings object.";
        settings_ = new QSettings();
    }
    return settings_;
}

QString SettingsModel::arma3Path()
{
    return settings()->value("arma3Dir", PathFinder::arma3Path()).toString();
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
    return settings()->value("teamSpeak3Path", PathFinder::teamspeak3Path()).toString();
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
    Global::sync->setMaxDownload(value.toInt());
}

QString SettingsModel::maxDownload()
{
    return settings()->value("maxDownload", "").toString();
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
    if (Global::sync == nullptr)
    {
        DBG << "ERROR: sync is null";
        return;
    }
    Global::sync->setPort(port.toInt());
}

QString SettingsModel::port()
{
    return settings()->value("port", QString::number(Constants::DEFAULT_PORT)).toString();
}

void SettingsModel::resetPort()
{
    settings()->setValue("port", QString::number(Constants::DEFAULT_PORT));
}

void SettingsModel::setMaxUpload(const QString& value)
{
    DBG;
    if (Global::sync == nullptr)
    {
        return;
    }
    Global::sync->setMaxUpload(value.toInt());
    settings()->setValue("maxUpload", value);
}

bool SettingsModel::battlEyeEnabled()
{
    return settings()->value("battlEyeEnabled", true).toBool();
}

void SettingsModel::setDeltaPatchingEnabled(bool enabled)
{
    settings()->setValue("deltaPatchingEnabled", enabled);
}

bool SettingsModel::deltaPatchingEnabled()
{
    return settings()->value("deltaPatchingEnabled", true).toBool();
}

void SettingsModel::setBattlEyeEnabled(bool enabled)
{
    settings()->setValue("battlEyeEnabled", enabled);
}

QString SettingsModel::modDownloadPath()
{
    return settings()->value("modDownloadPath", PathFinder::arma3Path()).toString();
}

void SettingsModel::setModDownloadPath(const QString& path)
{
    DBG << "path =" << path;
    if (!saveDir("modDownloadPath", path))
    {
        DBG << "Warning: failed to set mod download path. modDownloadPath() ="
            << modDownloadPath() << " path =" << path;
        return;
    }
    settings()->setValue("modDownloadPath", path);
    if (Global::model != nullptr)
    {
        Global::model->reset();
    }
}

void SettingsModel::resetModDownloadPath()
{
    setModDownloadPath(PathFinder::arma3MyDocuments());
}

QString SettingsModel::launchParameters()
{
    return settings()->value("launchParameters", "-noSplash -world=empty -skipIntro").toString();
}

void SettingsModel::setLaunchParameters(const QString& parameters)
{
    settings()->setValue("launchParameters", parameters);
}
