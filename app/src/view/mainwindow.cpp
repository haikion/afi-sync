#include "mainwindow.h"

#include <chrono>

#include "../model/version.h"
#include "ui_mainwindow.h"

using namespace std::chrono_literals;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->topBar->setSettingsView(ui->settingsView);
    setWindowTitle(QStringLiteral("AFISync " VERSION_CHARS));
}

MainWindow::~MainWindow()
{
    delete ui;
}

AsTreeWidget* MainWindow::treeWidget()
{
    return ui->treeWidget;
}

void MainWindow::init(IBandwidthMeter* bwMeter, ISettings* settingsModel)
{
    this->bwMeter = bwMeter;
    ui->settingsView->init(settingsModel);

    updateTimer.setInterval(1s);
    connect(&updateTimer, &QTimer::timeout, this, &MainWindow::update);
    updateTimer.start();
}

void MainWindow::update()
{
    ui->dlValueLabel->setText(bwMeter->downloadStr());
    ui->ulValueLabel->setText(bwMeter->uploadStr());
    ui->treeWidget->update();
}
