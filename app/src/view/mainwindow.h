#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "astreewidget.h"
#include "../model/interfaces/ibandwidthmeter.h"
#include "../model/interfaces/isettings.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow() override;

    AsTreeWidget* treeWidget();
    void init(IBandwidthMeter* bwMeter, ISettings* settingsModel);

private slots:
    void update();

private:
    Ui::MainWindow* ui;
    QTimer updateTimer;
    IBandwidthMeter* bwMeter;
};

#endif // MAINWINDOW_H
