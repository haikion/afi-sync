#ifndef ISETTINGS_H
#define ISETTINGS_H

#include <QString>

class ISettings
{
public:
    virtual QString arma3Path() = 0;
    virtual QString defaultArma3Path() = 0;
    virtual void setArma3Path(QString path) = 0;

    virtual QString teamSpeak3Path() = 0;
    virtual QString defaultTeamSpeak3Path() = 0;
    virtual void setTeamSpeak3Path(QString teamSpeak3Path) = 0;

    virtual QString steamPath() = 0;
    virtual QString defaultSteamPath() = 0;
    virtual void setSteamPath(QString steamPath) = 0;

    virtual QString modDownloadPath() = 0;
    virtual QString defaultModDownloadPath() = 0;
    virtual void setModDownloadPath(QString modDownloadPath) = 0;

    virtual QString launchParameters() = 0;
    virtual QString defaultLaunchParameters() = 0;
    virtual void setLaunchParameters(QString launchParameters) = 0;

    virtual QString port() = 0;
    virtual QString defaultPort() = 0;
    virtual void setPort(QString port) = 0;

    virtual void fixKickedFromServer() = 0;
    virtual void reportBug() = 0;
    virtual void forceActivate() = 0;

    virtual QString downloadLimit() = 0;
    virtual bool downloadLimitEnabled() = 0;
    virtual void setDownloadLimitEnabled(bool enabled) = 0;
    virtual QString defaultDownloadLimit() = 0;
    virtual void setDownloadLimit(QString downloadLimit) = 0;

    virtual QString uploadLimit() = 0;
    virtual bool uploadLimitEnabled() = 0;
    virtual void setUploadLimitEnabled(bool enabled) = 0;
    virtual QString defaultUploadLimit() = 0;
    virtual void setUploadLimit(QString uploadLimit) = 0;
};

#endif // ISETTINGS_H
