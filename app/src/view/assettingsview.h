#ifndef ASSETTINGSVIEW_H
#define ASSETTINGSVIEW_H

#include <QWidget>
#include <QTimer>
#include "../model/interfaces/isettings.h"

namespace Ui {
    class AsSettingsView;
}

class AsSettingsView : public QWidget
{
    Q_OBJECT

public:
    explicit AsSettingsView(QWidget* parent = nullptr);
    ~AsSettingsView() override;

    void init(ISettings* settingsModel);

private slots:
    void setArma3Path(const QString& path);
    void setSteamPath(const QString& path);
    void setModDownloadPath(const QString& path);
    void setTeamSpeak3Path(const QString& path);

    void on_parametersLineEdit_editingFinished();
    void on_portLineEdit_editingFinished();
    void on_reportButton_clicked();
    void on_deltaPatchingCheckbox_toggled(bool checked);

private:
    Ui::AsSettingsView *ui;
    ISettings* settingsModel;
};

#endif // ASSETTINGSVIEW_H
