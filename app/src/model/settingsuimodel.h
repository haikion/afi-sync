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

    [[nodiscard]] QString arma3Path() override;
    void resetArma3Path() override;
    void setArma3Path(const QString& path) override;

    [[nodiscard]] QString teamSpeak3Path() override;
    void resetTeamSpeak3Path() override;
    void setTeamSpeak3Path(const QString& teamSpeak3Path) override;

    [[nodiscard]] QString steamPath() override;
    void resetSteamPath() override;
    void setSteamPath(const QString& steamPath) override;

    [[nodiscard]] QString modDownloadPath() override;
    void resetModDownloadPath() override;
    void setModDownloadPath(const QString& modDownloadPath) override;

    [[nodiscard]] QString launchParameters() override;
    void setLaunchParameters(const QString& launchParameters) override;

    [[nodiscard]] QString port() override;
    void setPort(const QString &port) override;

    [[nodiscard]] QString maxDownload() override;
    [[nodiscard]] bool maxDownloadEnabled() override;
    void setMaxDownloadEnabled(bool maxDownloadEnabled) override;
    void setMaxDownload(const QString& maxDownload) override;

    [[nodiscard]] QString maxUpload() override;
    [[nodiscard]] bool maxUploadEnabled() override;
    void setMaxUploadEnabled(bool maxUploadEnabled) override;
    void setMaxUpload(const QString& maxUpload) override;

    [[nodiscard]] bool deltaPatchingEnabled() const override;
    void setDeltaPatchingEnabled(bool enabled) override;

private:
    TreeModel* treeModel;
};

#endif // SETTINGSUIMODEL_H
