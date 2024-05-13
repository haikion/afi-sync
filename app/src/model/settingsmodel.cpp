#include "settingsmodel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringLiteral>

#include "afisynclogger.h"
#include "global.h"
#include "paths.h"

using namespace Qt::StringLiterals;

SettingsModel& SettingsModel::instance()
{
    static SettingsModel ins;
    return ins;
}

//Saves only valid dirs
bool SettingsModel::saveDir(const QString& key, const QString& path)
{
    QFileInfo dir(path);
    QString originalPath = settings_->value(key).toString();
    if (originalPath == path || !dir.isDir() || !dir.isWritable())
    {
        return false;
    }
    auto qtStylePath = QDir::fromNativeSeparators(key);
    settings_->setValue(qtStylePath, path);
    return true;
}

//Sets default value for performance reasons.
QString SettingsModel::setting(const QString& key, const QString& defaultValue)
{
    QString set = settings_->value(key).toString();
    if (set.isEmpty())
    {
        LOG << "Setting default value " << defaultValue << " for " << key;
        settings_->setValue(key, defaultValue);
        set = defaultValue;
    }
    return set;
}

QString SettingsModel::arma3Path()
{
    auto val = setting(u"arma3Dir"_s, Paths::arma3Path());
    return QDir::fromNativeSeparators(val);
}

void SettingsModel::setArma3Path(const QString& path)
{
    if (!saveDir(u"arma3Dir"_s, path))
    {
        LOG_WARNING << "Failed to save Arma 3 path: " << path;
    }
}

void SettingsModel::resetArma3Path()
{
    settings_->setValue("arma3Dir", Paths::arma3Path());
}

QString SettingsModel::teamSpeak3Path()
{
    auto val = setting(u"teamSpeak3Path"_s, Paths::teamspeak3Path());
    return QDir::fromNativeSeparators(val);
}

void SettingsModel::setTeamSpeak3Path(const QString& path)
{
    if (!saveDir(u"teamSpeak3Path"_s, path))
    {
        LOG_WARNING << "Failed to save TeamSpeak 3 path: " << path;
    }
}

void SettingsModel::resetTeamSpeak3Path()
{
    settings_->setValue("teamSpeak3Path", Paths::teamspeak3Path());
}

QString SettingsModel::steamPath() const
{
    return QDir::fromNativeSeparators(settings_->value("steamPath", Paths::steamPath()).toString());
}

void SettingsModel::setSteamPath(const QString& path)
{
    settings_->setValue("steamPath", path);
}

void SettingsModel::resetSteamPath()
{
    LOG;
    settings_->setValue("steamPath", Paths::steamPath());
}

QString SettingsModel::maxUpload() const
{
    QString rVal = settings_->value("maxUpload", "").toString();
    return rVal;
}

void SettingsModel::setMaxDownload(const QString& maxDownload)
{
    settings_->setValue("maxDownload", maxDownload);
    setMaxDownloadSync();
}

QString SettingsModel::maxDownload() const
{
    return settings_->value("maxDownload", "").toString();
}

bool SettingsModel::maxDownloadEnabled()
{
    return settings_->value("maxDownloadEnabled", false).toBool();
}

bool SettingsModel::maxUploadEnabled() const
{
    return settings_->value("maxUploadEnabled", false).toBool();
}

void SettingsModel::setPort(const QString& port, bool enabled)
{
    settings_->setValue("port", port);
    settings_->setValue("portEnabled", enabled);
    Global::sync->setPort(enabled ? port.toInt() : Constants::DEFAULT_PORT.toInt());
}

bool SettingsModel::portEnabled() const
{
    return settings_->value("portEnabled", false).toBool();
}

void SettingsModel::setMaxDownloadEnabled(const bool maxDownloadEnabled)
{
    settings_->setValue("maxDownloadEnabled", maxDownloadEnabled);
    setMaxDownloadSync();
}

void SettingsModel::initBwLimits()
{
    setMaxDownloadSync();
    setMaxUploadSync();
}

void SettingsModel::setMaxUploadEnabled(const bool maxUploadEnabled)
{
    settings_->setValue("maxUploadEnabled", maxUploadEnabled);
    setMaxUploadSync();
}

QString SettingsModel::port() const
{
    return settings_->value("port", Constants::DEFAULT_PORT).toString();
}

QString SettingsModel::settingsPath()
{
    return QCoreApplication::applicationDirPath() + "/settings";
}

void SettingsModel::setTicked(const QString& modName, QString repoName, bool value)
{
    QString repoStr = repoName.replace("/| "_L1, "_"_L1);
    QString key = modName.isEmpty() ? repoName + "/checked" : modName + "/" + repoStr + "ticked";
    settings_->setValue(key, value);
}

bool SettingsModel::ticked(const QString& modName, QString repoName) const
{
    QString repoStr = repoName.replace("/| "_L1, "_"_L1);
    QString key = modName.isEmpty() ? repoName + "/checked" : modName + "/" + repoStr + "ticked";
    return settings_->value(key, false).toBool();
}

void SettingsModel::setProcessed(const QString& name, const QString& value)
{
    settings_->setValue(name + "/processed", value);
}

QString SettingsModel::processed(const QString& name) const
{
    return settings_->value(name + "/processed").toString();
}

QString SettingsModel::syncSettingsPath()
{
    return settingsPath() + "/sync";
}

void SettingsModel::setMaxUpload(const QString& maxUpload)
{
    settings_->setValue("maxUpload", maxUpload);
    setMaxUploadSync();
}

void SettingsModel::setMaxUploadSync() const
{
    Global::sync->setMaxUpload(maxUploadEnabled() ? maxUpload().toInt() : 0);
}

void SettingsModel::setMaxDownloadSync()
{
    Global::sync->setMaxDownload(maxDownloadEnabled() ? maxDownload().toInt() : 0);
}

void SettingsModel::setDeltaPatchingEnabled(bool enabled)
{
    settings_->setValue("deltaPatchingEnabled", enabled);
    if (Global::sync) //TODO: Remove might be too defensive
    {
        if (enabled)
        {
            Global::sync->enableDeltaUpdates();
        }
        else
        {
            Global::sync->disableDeltaUpdates();
        }
    }
    else
    {
        LOG_ERROR << "Sync is null!";
        Q_ASSERT(false);
    }
}

bool SettingsModel::deltaPatchingEnabled() const
{
    return settings_->value("deltaPatchingEnabled"_L1, false).toBool();
}

QString SettingsModel::modDownloadPath() const
{
    return QDir::fromNativeSeparators(settings_->value("modDownloadPath"_L1, Paths::arma3Path()).toString());
}

void SettingsModel::setModDownloadPath(QString path)
{
    LOG << "path = " << path;
    path = QDir::fromNativeSeparators(path);
    if (!saveDir(u"modDownloadPath"_s, path)) {
        LOG_WARNING << "Failed to set mod download path. modDownloadPath() = "
            << modDownloadPath() << " path = " << path;
        return;
    }
    settings_->setValue("modDownloadPath", path);
    if (Global::model)
    {
        Global::model->moveFiles();
    }
}

void SettingsModel::resetModDownloadPath()
{
    setModDownloadPath(Paths::arma3Path());
}

QString SettingsModel::patchesDownloadPath() const
{
    return modDownloadPath() + u"/afisync_patches"_s;
}

QString SettingsModel::launchParameters() const
{
    return settings_->value("launchParameters", "-noSplash -world=empty -skipIntro").toString();
}

void SettingsModel::setLaunchParameters(const QString& parameters)
{
    settings_->setValue("launchParameters", parameters);
}

SettingsModel::SettingsModel()
{
    QCoreApplication::setOrganizationName(u"AFISync"_s);
    QCoreApplication::setOrganizationDomain(u"armafinland.fi"_s);
    QCoreApplication::setApplicationName(u"AFISync"_s);

    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, SettingsModel::settingsPath());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    settings_ = new QSettings(this);
}
