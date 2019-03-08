#ifndef ASBANNERBAR_H
#define ASBANNERBAR_H

#include <QWidget>
#include "assettingsview.h"

namespace Ui {
    class AsBannerBar;
}

class AsBannerBar : public QWidget
{
    Q_OBJECT

public:
    explicit AsBannerBar(QWidget *parent = 0);
    ~AsBannerBar();

    void setSettingsView(AsSettingsView* settingsView);

private slots:
    void on_settingsButton_clicked();

private:
    Ui::AsBannerBar *ui;
    AsSettingsView* settingsView;
};

#endif // ASBANNERBAR_H
