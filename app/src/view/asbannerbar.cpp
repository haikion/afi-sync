#include "asbannerbar.h"
#include "ui_asbannerbar.h"

AsBannerBar::AsBannerBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AsBannerBar)
{
    ui->setupUi(this);
}

AsBannerBar::~AsBannerBar()
{
    delete ui;
}

void AsBannerBar::setSettingsView(AsSettingsView* settingsView)
{
    this->settingsView = settingsView;
}

void AsBannerBar::on_settingsButton_clicked()
{
    if (settingsView->isHidden())
    {
        settingsView->show();
        return;
    }
    settingsView->hide();
}
