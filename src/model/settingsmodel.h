//Model for settings view
#ifndef SETTINGSMODEL_H
#define SETTINGSMODEL_H

#include <QSettings>
#include <QObject>

class SettingsModel: public QObject
{
    Q_OBJECT

public:
    SettingsModel(QObject* parent = 0);

    bool resetBtsync();

public slots:
    static QString launchParameters();
    void setLaunchParameters(const QString& parameters);
    static bool battlEyeEnabled();
    void setBattlEyeEnabled(bool enabled);
    static QString modDownloadPath();
    static void setModDownloadPath(const QString& path);
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
    static void setMaxDownload(const QString& value);
    static QString maxDownload();

private:
    static QSettings* settings_;
    static bool saveDir(const QString& key, const QString& path);
    static QSettings* settings();
};

#endif // SETTINGSMODEL_H