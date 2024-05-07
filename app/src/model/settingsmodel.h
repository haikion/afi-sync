//Model for settings view
#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QSettings>
#include <QObject>

class SettingsModel: public QObject
{
    Q_OBJECT

public:
    SettingsModel(QObject* parent = nullptr);

public slots:
    [[nodiscard]] static QString launchParameters();
    static void setLaunchParameters(const QString& parameters);
    [[nodiscard]] static bool deltaPatchingEnabled();
    static void setDeltaPatchingEnabled(bool enabled);
    [[nodiscard]] static QString modDownloadPath();
    static void setModDownloadPath(QString path);
    static void resetModDownloadPath(); 
    [[nodiscard]] static QString patchesDownloadPath();
    [[nodiscard]] static QString arma3Path();
    static void setArma3Path(const QString& path);
    static void resetArma3Path();
    [[nodiscard]] static QString teamSpeak3Path();
    static void setTeamSpeak3Path(const QString& path);
    static void resetTeamSpeak3Path();
    [[nodiscard]] static QString steamPath();
    static void setSteamPath(const QString& path);
    static void resetSteamPath();
    static void setMaxUpload(const QString& maxUpload);
    [[nodiscard]] static QString maxUpload();
    [[nodiscard]] static bool maxUploadEnabled();
    static void setMaxDownload(const QString& maxDownload);
    [[nodiscard]] static QString maxDownload();
    static bool maxDownloadEnabled();
    static void setInstallDate(const QString& repoName, qint64 value);
    [[nodiscard]] static unsigned installDate(const QString& repoName);
    static void setPort(const QString& port, bool enabled);
    [[nodiscard]] static QString port();
    [[nodiscard]] static QString syncSettingsPath();
    [[nodiscard]] static QString settingsPath();
    //Tells if syncitem is ticked (!= files checked)
    static void setTicked(const QString& modName, QString repoName, bool value);
    [[nodiscard]] static bool ticked(const QString& modName, QString repoName);
    //Tells if process completion (file checking, installation) is needed
    static void setProcessed(const QString& name, const QString& value);
    [[nodiscard]] static QString processed(const QString& name);
    [[nodiscard]] static bool portEnabled();
    static void setMaxUploadEnabled(bool maxUploadEnabled);
    static void setMaxDownloadEnabled(bool maxDownloadEnabled);
    static void initBwLimits();

private:
    static QSettings* settings_;
    static bool saveDir(const QString& key, const QString& path);
    static QString setting(const QString& key, const QString& defaultValue);
    static QString syncSettingsPath_;
    static QString settingsPath_;
    static QSettings* settings();
    static void createSettings();
    static void setMaxUploadSync();
    static void setMaxDownloadSync();
};

#endif // SETTINGSMODEL_H
