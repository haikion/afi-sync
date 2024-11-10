//Model for settings view
#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QSettings>
#include <QObject>

class SettingsModel: public QObject
{
    Q_OBJECT

public:
    SettingsModel(const SettingsModel&) = delete;
    SettingsModel operator=(const SettingsModel&) = delete;
    ~SettingsModel() override = default;

    static SettingsModel& instance();

    [[nodiscard]] QString launchParameters() const;
    void setLaunchParameters(const QString& parameters);
    [[nodiscard]] bool versionCheckEnabled() const;
    [[nodiscard]] bool deltaPatchingEnabled() const;
    [[nodiscard]] QString modDownloadPath() const;
    void setModDownloadPath(QString path);
    void resetModDownloadPath();
    [[nodiscard]] QString patchesDownloadPath() const;
    [[nodiscard]] QString arma3Path();
    void setArma3Path(const QString& path);
    void resetArma3Path();
    [[nodiscard]] QString teamSpeak3Path();
    void setTeamSpeak3Path(const QString& path);
    void resetTeamSpeak3Path();
    [[nodiscard]] QString steamPath() const;
    void setSteamPath(const QString& path);
    void resetSteamPath();
    void setMaxUpload(const QString& maxUpload);
    [[nodiscard]] QString maxUpload() const;
    [[nodiscard]] bool maxUploadEnabled() const;
    void setMaxDownload(const QString& maxDownload);
    [[nodiscard]] QString maxDownload() const;
    bool maxDownloadEnabled();
    void setPort(const QString& port, bool enabled);
    [[nodiscard]] QString port() const;
    [[nodiscard]] static QString syncSettingsPath();
    [[nodiscard]] static QString settingsPath();
    //Tells if syncitem is ticked (!= files checked)
    void setTicked(const QString& modName, QString repoName, bool value);
    [[nodiscard]] bool ticked(const QString& modName, QString repoName) const;
    //Tells if process completion (file checking, installation) is needed
    void setProcessed(const QString& name, const QString& value);
    [[nodiscard]] QString processed(const QString& name) const;
    [[nodiscard]] bool portEnabled() const;
    void setMaxUploadEnabled(bool maxUploadEnabled);
    void setMaxDownloadEnabled(bool maxDownloadEnabled);
    void initBwLimits();

public slots:
    void setVersionCheckEnabled(bool enabled);
    void setDeltaPatchingEnabled(bool enabled);

private:
    QSettings* settings_{nullptr};
    QString setting(const QString& key, const QString& defaultValue);
    QString settingsPath_;
    QString syncSettingsPath_;
    bool saveDir(const QString& key, const QString& path);

    SettingsModel();

    void createSettings();
    void setMaxUploadSync() const;
    void setMaxDownloadSync();
};

#endif // SETTINGSMODEL_H
