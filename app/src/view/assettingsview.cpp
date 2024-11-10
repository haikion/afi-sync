#include <QDesktopServices>
#include <QIntValidator>
#include <QStringLiteral>

#include "assettingsview.h"
#include "model/settingsmodel.h"
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

void AsSettingsView::init()
{
    static SettingsModel& settingsModel = SettingsModel::instance();

    ui->parametersLineEdit->setText(settingsModel.launchParameters());
    connect(ui->parametersLineEdit, &QLineEdit::editingFinished, &settingsModel, [&]
    {
        settingsModel.setLaunchParameters(ui->parametersLineEdit->text());
    });

    ui->versionCheckBox->setChecked(settingsModel.versionCheckEnabled());
    connect(ui->versionCheckBox, &QCheckBox::toggled,
            &settingsModel, &SettingsModel::setVersionCheckEnabled);

    ui->steamPathSetting->init(u"Steam"_s, QDir::toNativeSeparators(settingsModel.steamPath()));
    connect(ui->steamPathSetting, &PathSetting::textEdited, &settingsModel, &SettingsModel::setSteamPath);
    connect(ui->steamPathSetting, &PathSetting::resetPressed, this, [&]
    {
        settingsModel.resetSteamPath();
        ui->steamPathSetting->setValue(settingsModel.steamPath());
    });

    ui->modsPathSetting->init(u"Mod Download"_s, QDir::toNativeSeparators(settingsModel.modDownloadPath()));
    connect(ui->modsPathSetting, &PathSetting::textEdited, &settingsModel, &SettingsModel::setModDownloadPath);
    connect(ui->modsPathSetting, &PathSetting::resetPressed, this, [&]
    {
        settingsModel.resetModDownloadPath();
        ui->modsPathSetting->setValue(settingsModel.modDownloadPath());
    });

    ui->teamspeakPathSetting->init(u"TeamSpeak 3"_s, QDir::toNativeSeparators(settingsModel.teamSpeak3Path()));
    connect(ui->teamspeakPathSetting, &PathSetting::textEdited, &settingsModel, &SettingsModel::setTeamSpeak3Path);
    connect(ui->teamspeakPathSetting, &PathSetting::resetPressed, this, [&]
    {
        settingsModel.resetTeamSpeak3Path();
        ui->teamspeakPathSetting->setValue(settingsModel.teamSpeak3Path());
    });

    ui->arma3PathSetting->init(u"Arma 3"_s, QDir::toNativeSeparators(settingsModel.arma3Path()));
    connect(ui->arma3PathSetting, &PathSetting::textEdited, &settingsModel, &SettingsModel::setArma3Path);
    connect(ui->arma3PathSetting, &PathSetting::resetPressed, this, [&]
    {
        settingsModel.resetArma3Path();
        ui->arma3PathSetting->setValue(settingsModel.arma3Path());
    });

    ui->downloadSetting->init(u"Download limit:"_s, settingsModel.maxDownload(), settingsModel.maxDownloadEnabled());
    connect(ui->downloadSetting, &OptionalSetting::checked, &settingsModel, &SettingsModel::setMaxDownloadEnabled);
    connect(ui->downloadSetting, &OptionalSetting::valueChanged, &settingsModel, &SettingsModel::setMaxDownload);

    ui->uploadSetting->init(u"Upload limit:"_s, settingsModel.maxUpload(), settingsModel.maxUploadEnabled());
    connect(ui->uploadSetting, &OptionalSetting::checked, &settingsModel, &SettingsModel::setMaxUploadEnabled);
    connect(ui->uploadSetting, &OptionalSetting::valueChanged, &settingsModel, &SettingsModel::setMaxUpload);

    ui->portLineEdit->setText(settingsModel.port());
    connect(ui->portLineEdit, &QLineEdit::editingFinished, &settingsModel, [=]
    {
        settingsModel.setPort(ui->portLineEdit->text(), true);
    });

    ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));
    ui->deltaPatchingCheckbox->setChecked(settingsModel.deltaPatchingEnabled());
    connect(ui->deltaPatchingCheckbox, &QCheckBox::toggled,
            &settingsModel, &SettingsModel::setDeltaPatchingEnabled);

    connect(ui->reportButton, &QPushButton::clicked, this, [=]
    {
        QDesktopServices::openUrl(QUrl::fromEncoded("https://form.jotformeu.com/61187638191361"));
    });
}
