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
    if (!dir.isDir() || !dir.isWritable())
    {
        return false;
    }
    settings()->setValue(key, path);
    return true;
}

QSettings*SettingsModel::settings()
{
    if (settings_ == nullptr)
    {
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
    QString rVal = settings()->value("maxUpload", "0").toString();
    DBG << "rVal =" << rVal;
    return rVal;
}

void SettingsModel::setMaxDownload(const QString& value)
{
    settings()->setValue("maxDownload", value);
    Global::btsync->setMaxDownload(value.toInt());
}

QString SettingsModel::maxDownload()
{
    return settings()->value("maxDownload", "0").toString();
}

void SettingsModel::setMaxUpload(const QString& value)
{
    DBG;
    if (Global::btsync == nullptr)
    {
        return;
    }
    Global::btsync->setMaxUpload(value.toInt());
    settings()->setValue("maxUpload", value);
}

bool SettingsModel::battlEyeEnabled()
{
    return settings()->value("battlEyeEnabled", true).toBool();
}

void SettingsModel::setBattlEyeEnabled(bool enabled)
{
    settings()->setValue("battlEyeEnabled", enabled);
}

QString SettingsModel::modDownloadPath()
{
    return settings()->value("modDownloadPath", PathFinder::arma3MyDocuments()).toString();
}

void SettingsModel::setModDownloadPath(const QString& path)
{
    DBG << "path =" << path;
    if (!saveDir("modDownloadPath", path))
    {
        return;
    }
    settings()->setValue("modDownloadPath", path);
    if (Global::model != nullptr)
    {
        Global::model->reset();
    }
}

bool SettingsModel::resetBtsync()
{
    return settings()->value("resetBtsync", false).toBool();
}

void SettingsModel::resetModDownloadPath()
{
    settings()->setValue("modDownloadPath", PathFinder::arma3MyDocuments());
}

QString SettingsModel::launchParameters()
{
    return settings()->value("launchParameters", "-noSplash -world=empty -skipIntro").toString();
}

void SettingsModel::setLaunchParameters(const QString& parameters)
{
    settings()->setValue("launchParameters", parameters);
}