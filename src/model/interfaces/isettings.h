#ifndef ISETTINGS_H
#define ISETTINGS_H

#include <QString>

class ISettings
{
public:
    virtual ~ISettings() = default;
    virtual QString arma3Path() = 0;
    virtual void resetArma3Path() = 0;
    virtual void setArma3Path(QString path) = 0;

    virtual QString teamSpeak3Path() = 0;
    virtual void resetTeamSpeak3Path() = 0;
    virtual void setTeamSpeak3Path(QString teamSpeak3Path) = 0;

    virtual QString steamPath() = 0;
    virtual void resetSteamPath() = 0;
    virtual void setSteamPath(QString steamPath) = 0;

    virtual QString modDownloadPath() = 0;
    virtual void resetModDownloadPath() = 0;
    virtual void setModDownloadPath(QString modDownloadPath) = 0;

    virtual QString launchParameters() = 0;
    virtual void setLaunchParameters(QString launchParameters) = 0;

    virtual QString port() = 0;
    virtual void setPort(QString port) = 0;

    virtual QString maxDownload() = 0;
    virtual bool maxDownloadEnabled() = 0;
    virtual void setMaxDownloadEnabled(const bool enabled) = 0;
    virtual void setMaxDownload(QString maxDownload) = 0;

    virtual QString maxUpload() = 0;
    virtual bool maxUploadEnabled() = 0;
    virtual void setMaxUploadEnabled(const bool enabled) = 0;
    virtual void setMaxUpload(const QString& maxUpload) = 0;

    virtual bool deltaPatchingEnabled() const = 0;
    virtual void setDeltaPatchingEnabled(const bool enabled) = 0;
};

#endif // ISETTINGS_H
