//Model for settings view
#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QSettings>
#include <QObject>

class SettingsModel: public QObject
{
    Q_OBJECT

public:
    SettingsModel(QObject* parent); //Should only be used when constructing for QQmlEngine

public slots:
    static QString launchParameters();
    void setLaunchParameters(const QString& parameters);
    static bool battlEyeEnabled();
    void setBattlEyeEnabled(bool enabled);
    static bool deltaPatchingEnabled();
    static void setDeltaPatchingEnabled(bool enabled);
    static QString modDownloadPath();
    static void setModDownloadPath(QString path);
    void resetModDownloadPath();
    static QString arma3Path();
    void setArma3Path(const QString& path);
    void resetArma3Path();
    static QString teamSpeak3Path();
    void setTeamSpeak3Path(const QString& path);
    void resetTeamSpeak3Path();
    static QString steamPath();
    static void setSteamPath(const QString& path);
    static void resetSteamPath();
    static void setMaxUpload(const QString& value);
    static QString maxUpload();
    static void setMaxUploadEnabled(bool value);
    static bool maxUploadEnabled();
    static void setMaxDownload(const QString& value);
    static QString maxDownload();
    static void setMaxDownloadEnabled(bool value);
    static bool maxDownloadEnabled();
    static void setInstallDate(const QString& repoName, const unsigned& value);
    static unsigned installDate(const QString& repoName);
    static void setPort(const QString& port);
    static QString port();
    static void resetPort();
    static QString syncSettingsPath();
    static QString settingsPath();
    //Tells if syncitem is ticked (!= files checked)
    static void setTicked(const QString& modName, QString repoName, bool value);
    static bool ticked(const QString& modName, QString repoName);
    //Tells if process completion (file checking, installation) is needed
    static void setProcess(const QString& name, bool value);
    static bool process(const QString& name);

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
