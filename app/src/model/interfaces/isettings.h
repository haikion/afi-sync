#ifndef ISETTINGS_H
#define ISETTINGS_H

#include <QObject>
#include <QString>

class ISettings: public QObject
{
    Q_OBJECT

public:
	ISettings() = default;
    ~ISettings() override = default;

    [[nodiscard]] virtual QString arma3Path() = 0;
    virtual void resetArma3Path() = 0;
    virtual void setArma3Path(const QString& path) = 0;

    [[nodiscard]] virtual QString teamSpeak3Path() = 0;
    virtual void resetTeamSpeak3Path() = 0;
    virtual void setTeamSpeak3Path(const QString& teamSpeak3Path) = 0;

    [[nodiscard]] virtual QString steamPath() = 0;
    virtual void resetSteamPath() = 0;
    virtual void setSteamPath(const QString& steamPath) = 0;

    [[nodiscard]] virtual QString modDownloadPath() = 0;
    virtual void resetModDownloadPath() = 0;
    virtual void setModDownloadPath(const QString& modDownloadPath) = 0;

    [[nodiscard]] virtual QString launchParameters() = 0;
    virtual void setLaunchParameters(const QString& launchParameters) = 0;

    [[nodiscard]] virtual QString port() = 0;
    virtual void setPort(const QString& port) = 0;

    [[nodiscard]] virtual QString maxDownload() = 0;
    virtual bool maxDownloadEnabled() = 0;
    virtual void setMaxDownloadEnabled(bool enabled) = 0;
    virtual void setMaxDownload(const QString& maxDownload) = 0;

    [[nodiscard]] virtual QString maxUpload() = 0;
    virtual bool maxUploadEnabled() = 0;
    virtual void setMaxUploadEnabled(bool enabled) = 0;
    virtual void setMaxUpload(const QString& maxUpload) = 0;

    [[nodiscard]] virtual bool deltaPatchingEnabled() const = 0;

public slots:
    virtual void setDeltaPatchingEnabled(bool enabled) = 0;

private:
    Q_DISABLE_COPY(ISettings)
};

#endif // ISETTINGS_H
