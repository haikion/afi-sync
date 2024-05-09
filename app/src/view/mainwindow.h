#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include "model/interfaces/ibandwidthmeter.h"
#include "astreewidget.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    AsTreeWidget* treeWidget();
    void init(IBandwidthMeter* bwMeter);

private slots:
    void update();

private:
    Ui::MainWindow* ui;
    QTimer updateTimer;
    IBandwidthMeter* bwMeter;
};

#endif // MAINWINDOW_H
