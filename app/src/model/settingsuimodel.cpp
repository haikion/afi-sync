#include "settingsmodel.h"
#include "settingsuimodel.h"

QString SettingsUiModel::arma3Path()
{
    return SettingsModel::arma3Path();
}

void SettingsUiModel::resetArma3Path()
{
    SettingsModel::resetArma3Path();
}

void SettingsUiModel::setArma3Path(QString path)
{
    SettingsModel::setArma3Path(path);
}

QString SettingsUiModel::teamSpeak3Path()
{
    return SettingsModel::teamSpeak3Path();
}

void SettingsUiModel::resetTeamSpeak3Path()
{
    SettingsModel::resetTeamSpeak3Path();
}

void SettingsUiModel::setTeamSpeak3Path(QString teamSpeak3Path)
{
    SettingsModel::setTeamSpeak3Path(teamSpeak3Path);
}

QString SettingsUiModel::steamPath()
{
    return SettingsModel::steamPath();
}

void SettingsUiModel::resetSteamPath()
{
    SettingsModel::resetSteamPath();
}

void SettingsUiModel::setSteamPath(QString steamPath)
{
    SettingsModel::setSteamPath(steamPath);
}

QString SettingsUiModel::modDownloadPath()
{
    return SettingsModel::modDownloadPath();
}

void SettingsUiModel::resetModDownloadPath()
{
    SettingsModel::resetModDownloadPath();
}

void SettingsUiModel::setModDownloadPath(QString modDownloadPath)
{
    SettingsModel::setModDownloadPath(modDownloadPath);
}

QString SettingsUiModel::launchParameters()
{
    return SettingsModel::launchParameters();
}

void SettingsUiModel::setLaunchParameters(QString launchParameters)
{
    SettingsModel::setLaunchParameters(launchParameters);
}

QString SettingsUiModel::port()
{
    return SettingsModel::port();
}

void SettingsUiModel::setPort(QString port)
{
    SettingsModel::setPort(port, true);
}

QString SettingsUiModel::maxDownload()
{
    return SettingsModel::maxDownload();
}

bool SettingsUiModel::maxDownloadEnabled()
{
    return SettingsModel::maxDownloadEnabled();
}

void SettingsUiModel::setMaxDownloadEnabled(const bool maxDownloadEnabled)
{
    SettingsModel::setMaxDownloadEnabled(maxDownloadEnabled);
}

void SettingsUiModel::setMaxDownload(QString downloadLimit)
{
    SettingsModel::setMaxDownload(downloadLimit);
}

QString SettingsUiModel::maxUpload()
{
    return SettingsModel::maxUpload();
}

bool SettingsUiModel::maxUploadEnabled()
{
    return SettingsModel::maxUploadEnabled();
}

void SettingsUiModel::setMaxUploadEnabled(const bool maxUploadEnabled)
{
    SettingsModel::setMaxUploadEnabled(maxUploadEnabled);
}

void SettingsUiModel::setMaxUpload(const QString& uploadLimit)
{
    SettingsModel::setMaxUpload(uploadLimit);
}

bool SettingsUiModel::deltaPatchingEnabled() const
{
    return SettingsModel::deltaPatchingEnabled();
}

void SettingsUiModel::setDeltaPatchingEnabled(const bool enabled)
{
    SettingsModel::setDeltaPatchingEnabled(enabled);
}
