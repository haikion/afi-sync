//Model for settings view
#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QSettings>
#include <QObject>

class SettingsModel: public QObject
{
    Q_OBJECT

public:
    SettingsModel(QObject* parent = nullptr); //Should only be used when constructing for QQmlEngine

public slots:
    static QString launchParameters();
    static void setLaunchParameters(const QString& parameters);
    static bool battlEyeEnabled(); //TODO: Remove?
    void setBattlEyeEnabled(bool enabled); //TODO: Remove?
    static bool deltaPatchingEnabled();
    static void setDeltaPatchingEnabled(bool enabled);
    static QString modDownloadPath();
    static void setModDownloadPath(QString path);
    static void resetModDownloadPath();
    static QString arma3Path();
    static void setArma3Path(const QString& path);
    static void resetArma3Path();
    static QString teamSpeak3Path();
    static void setTeamSpeak3Path(const QString& path);
    static void resetTeamSpeak3Path();
    static QString steamPath();
    static void setSteamPath(const QString& path);
    static void resetSteamPath();
    static void setMaxUpload(const QString& maxUpload, bool enabled);
    static QString maxUpload();
    static bool maxUploadEnabled();
    static void setMaxDownload(const QString& maxDownload, bool enabled);
    static QString maxDownload();
    static bool maxDownloadEnabled();
    static void setInstallDate(const QString& repoName, const unsigned& value);
    static unsigned installDate(const QString& repoName);
    static void setPort(const QString& port, bool enabled);
    static QString port();
    static QString syncSettingsPath();
    static QString settingsPath();
    //Tells if syncitem is ticked (!= files checked)
    static void setTicked(const QString& modName, QString repoName, bool value);
    static bool ticked(const QString& modName, QString repoName);
    //Tells if process completion (file checking, installation) is needed
    static void setProcess(const QString& name, bool value);
    static bool process(const QString& name);
    static bool portEnabled();
    static void setMaxUploadEnabled(const bool maxUploadEnabled);
    static void setMaxDownloadEnabled(const bool maxDownloadEnabled);

private:
    static QSettings* settings_;
    static bool saveDir(const QString& key, const QString& path);
    static QString setting(const QString& key, const QString& defaultValue);
    static QString syncSettingsPath_;
    static QString settingsPath_;
    static QSettings* settings();
    static void createSettings();
};

#endif // SETTINGSMODEL_H
