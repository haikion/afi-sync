#ifndef SETTINGSUIMODEL_H
#define SETTINGSUIMODEL_H

#include "interfaces/isettings.h"
#include "treemodel.h"

/**
 * @brief QWidget compatibility class for settings
 */
class SettingsUiModel : virtual public ISettings
{
public:
    SettingsUiModel() = default;

    virtual QString arma3Path();
    virtual void resetArma3Path();
    virtual void setArma3Path(QString path);

    virtual QString teamSpeak3Path();
    virtual void resetTeamSpeak3Path();
    virtual void setTeamSpeak3Path(QString teamSpeak3Path);

    virtual QString steamPath();
    virtual void resetSteamPath();
    virtual void setSteamPath(QString steamPath);

    virtual QString modDownloadPath();
    virtual void resetModDownloadPath();
    virtual void setModDownloadPath(QString modDownloadPath);

    virtual QString launchParameters();
    virtual void setLaunchParameters(QString launchParameters);

    virtual QString port();
    virtual void setPort(QString port);

    virtual QString maxDownload();
    virtual bool maxDownloadEnabled();
    virtual void setMaxDownloadEnabled(const bool maxDownloadEnabled);
    virtual void setMaxDownload(QString maxDownload);

    virtual QString maxUpload();
    virtual bool maxUploadEnabled();
    virtual void setMaxUploadEnabled(const bool maxUploadEnabled);
    virtual void setMaxUpload(const QString& maxUpload);

    virtual bool deltaPatchingEnabled() const;
    virtual void setDeltaPatchingEnabled(const bool enabled);

private:
    TreeModel* treeModel;
};

#endif // SETTINGSUIMODEL_H
