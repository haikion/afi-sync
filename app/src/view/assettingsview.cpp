#include <QIntValidator>
#include <QDesktopServices>
#include "assettingsview.h"
#include "pathsetting.h"
#include "optionalsetting.h"
#include "ui_assettingsview.h"

AsSettingsView::AsSettingsView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AsSettingsView)
{
    ui->setupUi(this);
    hide();
}

AsSettingsView::~AsSettingsView()
{
    delete ui;
}

void AsSettingsView::init(ISettings* settingsModel)
{
    this->settingsModel = settingsModel;
    ui->parametersLineEdit->setText(settingsModel->launchParameters());

    ui->steamPathSetting->init("Steam", settingsModel->steamPath());
    connect(ui->steamPathSetting, &PathSetting::textEdited, this, &AsSettingsView::setSteamPath);
    connect(ui->steamPathSetting, &PathSetting::resetPressed, [=]
    {
        settingsModel->resetSteamPath();
        ui->steamPathSetting->setValue(settingsModel->steamPath());
    });

    ui->modsPathSetting->init("Mod Download", settingsModel->modDownloadPath());
    connect(ui->modsPathSetting, &PathSetting::textEdited, this, &AsSettingsView::setModDownloadPath);
    connect(ui->modsPathSetting, &PathSetting::resetPressed, [=]
    {
        settingsModel->resetModDownloadPath();
        ui->modsPathSetting->setValue(settingsModel->modDownloadPath());
    });

    ui->teamspeakPathSetting->init("TeamSpeak 3", settingsModel->teamSpeak3Path());
    connect(ui->teamspeakPathSetting, &PathSetting::textEdited, this, &AsSettingsView::setTeamSpeak3Path);
    connect(ui->teamspeakPathSetting, &PathSetting::resetPressed, [=]
    {
        settingsModel->resetTeamSpeak3Path();
        ui->teamspeakPathSetting->setValue(settingsModel->teamSpeak3Path());
    });

    ui->arma3PathSetting->init("Arma 3", settingsModel->arma3Path());
    connect(ui->arma3PathSetting, &PathSetting::textEdited, this, &AsSettingsView::setArma3Path);
    connect(ui->arma3PathSetting, &PathSetting::resetPressed, [=] {
        settingsModel->resetArma3Path();
        ui->arma3PathSetting->setValue(settingsModel->arma3Path());
    });

    ui->downloadSetting->init("Download limit:", settingsModel->maxDownload(), settingsModel->maxDownloadEnabled());
    connect(ui->downloadSetting, &OptionalSetting::checked, [=] (bool ticked)
    {
        settingsModel->setMaxDownloadEnabled(ticked);
    });
    connect(ui->downloadSetting, &OptionalSetting::valueChanged, [=] (QString newValue)
    {
        settingsModel->setMaxDownload(newValue);
    });
    ui->uploadSetting->init("Upload limit:", settingsModel->maxUpload(), settingsModel->maxUploadEnabled());
    connect(ui->uploadSetting, &OptionalSetting::checked, [=] (bool ticked) {
        settingsModel->setMaxUploadEnabled(ticked);
    });
    connect(ui->uploadSetting, &OptionalSetting::valueChanged, [=] (QString newValue)
    {
        settingsModel->setMaxUpload(newValue);
    });

    ui->portLineEdit->setText(settingsModel->port());
    ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));
    ui->deltaPatchingCheckbox->setChecked(settingsModel->deltaPatchingEnabled());
}

void AsSettingsView::setArma3Path(QString path)
{
    settingsModel->setArma3Path(path);
}

void AsSettingsView::setSteamPath(QString path)
{
    settingsModel->setSteamPath(path);
}

void AsSettingsView::setModDownloadPath(QString path)
{
    settingsModel->setModDownloadPath(path);
}

void AsSettingsView::setTeamSpeak3Path(QString path)
{
    settingsModel->setTeamSpeak3Path(path);
}

void AsSettingsView::on_parametersLineEdit_editingFinished()
{
    settingsModel->setLaunchParameters(ui->parametersLineEdit->text());
}

void AsSettingsView::on_portLineEdit_editingFinished()
{
    settingsModel->setPort(ui->portLineEdit->text());
}

void AsSettingsView::on_reportButton_clicked()
{
    QDesktopServices::openUrl(QUrl::fromEncoded("https://form.jotformeu.com/61187638191361"));
}

void AsSettingsView::on_deltaPatchingCheckbox_toggled(const bool checked)
{
    settingsModel->setDeltaPatchingEnabled(checked);
}
