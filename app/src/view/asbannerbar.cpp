#include "asbannerbar.h"

#include "ui_asbannerbar.h"

AsBannerBar::AsBannerBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AsBannerBar)
{
    ui->setupUi(this);
    connect(ui->settingsButton, &QPushButton::pressed, this, [=]
    {
        if (settingsView->isHidden())
        {
            settingsView->show();
            return;
        }
        settingsView->hide();
    });
}

AsBannerBar::~AsBannerBar()
{
    delete ui;
}

void AsBannerBar::setSettingsView(AsSettingsView* settingsView)
{
    this->settingsView = settingsView;
}
