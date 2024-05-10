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
    explicit AsBannerBar(QWidget* parent = nullptr);
    ~AsBannerBar() override;

    void setSettingsView(AsSettingsView* settingsView);

private:
    Ui::AsBannerBar* ui;
    AsSettingsView* settingsView{nullptr};
};

#endif // ASBANNERBAR_H
