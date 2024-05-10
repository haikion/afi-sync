#ifndef ASSETTINGSVIEW_H
#define ASSETTINGSVIEW_H

#include <QWidget>
#include <QTimer>

#include "model/interfaces/isettings.h"

namespace Ui {
    class AsSettingsView;
}

class AsSettingsView : public QWidget
{
    Q_OBJECT

public:
    explicit AsSettingsView(QWidget* parent = nullptr);
    ~AsSettingsView() override;

    void init();

private:
    Ui::AsSettingsView* ui;
    ISettings* settingsModel{nullptr};
};

#endif // ASSETTINGSVIEW_H
