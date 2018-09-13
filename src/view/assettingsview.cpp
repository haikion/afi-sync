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
    ui->arma3PathSetting->init("Arma 3", settingsModel->arma3Path());
    ui->modsPathSetting->init("Mod Download", settingsModel->modDownloadPath());
    ui->teamspeakPathSetting->init("TeamSpeak 3", settingsModel->teamSpeak3Path());
    ui->steamPathSetting->init("Steam", settingsModel->steamPath());
    ui->downloadSetting->init("Download limit:", settingsModel->maxDownload(), settingsModel->maxDownloadEnabled());
    ui->uploadSetting->init("Upload limit:", settingsModel->maxUpload(), settingsModel->maxUploadEnabled());
    ui->portLineEdit->setText(settingsModel->port());
    ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));

    connect(ui->arma3PathSetting, SIGNAL(textEdited(QString)), this, SLOT(setArma3Path(QString)));
    connect(ui->steamPathSetting, SIGNAL(textEdited(QString)), this, SLOT(setSteamPath(QString)));
    connect(ui->downloadSetting, &OptionalSetting::checked, [=] (bool newValue) {
        settingsModel->setMaxDownloadEnabled(newValue);
    });
    connect(ui->uploadSetting, &OptionalSetting::checked, [=] (bool newValue) {
        settingsModel->setMaxUploadEnabled(newValue);
    });
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
    QDesktopServices::openUrl(QUrl::fromEncoded("https://www.google.fi"));
}
