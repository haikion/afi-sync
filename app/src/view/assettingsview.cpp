#include <QDesktopServices>
#include <QIntValidator>
#include <QStringLiteral>

#include "assettingsview.h"
#include "optionalsetting.h"
#include "pathsetting.h"
#include "ui_assettingsview.h"

using namespace Qt::StringLiterals;

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

    ui->steamPathSetting->init(u"Steam"_s, settingsModel->steamPath());
    connect(ui->steamPathSetting, &PathSetting::textEdited, this, &AsSettingsView::setSteamPath);
    connect(ui->steamPathSetting, &PathSetting::resetPressed, this, [=]
    {
        settingsModel->resetSteamPath();
        ui->steamPathSetting->setValue(settingsModel->steamPath());
    });

    ui->modsPathSetting->init(u"Mod Download"_s, settingsModel->modDownloadPath());
    connect(ui->modsPathSetting, &PathSetting::textEdited, this, &AsSettingsView::setModDownloadPath);
    connect(ui->modsPathSetting, &PathSetting::resetPressed, this, [=]
    {
        settingsModel->resetModDownloadPath();
        ui->modsPathSetting->setValue(settingsModel->modDownloadPath());
    });

    ui->teamspeakPathSetting->init(u"TeamSpeak 3"_s, settingsModel->teamSpeak3Path());
    connect(ui->teamspeakPathSetting, &PathSetting::textEdited, this, &AsSettingsView::setTeamSpeak3Path);
    connect(ui->teamspeakPathSetting, &PathSetting::resetPressed, this, [=]
    {
        settingsModel->resetTeamSpeak3Path();
        ui->teamspeakPathSetting->setValue(settingsModel->teamSpeak3Path());
    });

    ui->arma3PathSetting->init(u"Arma 3"_s, settingsModel->arma3Path());
    connect(ui->arma3PathSetting, &PathSetting::textEdited, this, &AsSettingsView::setArma3Path);
    connect(ui->arma3PathSetting, &PathSetting::resetPressed, this, [=] {
        settingsModel->resetArma3Path();
        ui->arma3PathSetting->setValue(settingsModel->arma3Path());
    });

    ui->downloadSetting->init(u"Download limit:"_s,
                              settingsModel->maxDownload(),
                              settingsModel->maxDownloadEnabled());
    connect(ui->downloadSetting, &OptionalSetting::checked, this, [=] (bool ticked)
    {
        settingsModel->setMaxDownloadEnabled(ticked);
    });
    connect(ui->downloadSetting, &OptionalSetting::valueChanged, this, [=] (const QString& newValue)
    {
        settingsModel->setMaxDownload(newValue);
    });
    ui->uploadSetting->init(u"Upload limit:"_s,
                            settingsModel->maxUpload(),
                            settingsModel->maxUploadEnabled());
    connect(ui->uploadSetting, &OptionalSetting::checked, this, [=] (bool ticked) {
        settingsModel->setMaxUploadEnabled(ticked);
    });
    connect(ui->uploadSetting, &OptionalSetting::valueChanged, this, [=] (const QString& newValue)
    {
        settingsModel->setMaxUpload(newValue);
    });

    ui->portLineEdit->setText(settingsModel->port());
    ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));
    ui->deltaPatchingCheckbox->setChecked(settingsModel->deltaPatchingEnabled());
}

void AsSettingsView::setArma3Path(const QString& path)
{
    settingsModel->setArma3Path(path);
}

void AsSettingsView::setSteamPath(const QString& path)
{
    settingsModel->setSteamPath(path);
}

void AsSettingsView::setModDownloadPath(const QString& path)
{
    settingsModel->setModDownloadPath(path);
}

void AsSettingsView::setTeamSpeak3Path(const QString& path)
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

void AsSettingsView::on_deltaPatchingCheckbox_toggled(bool checked)
{
    settingsModel->setDeltaPatchingEnabled(checked);
}
