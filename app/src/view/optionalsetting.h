#ifndef OPTIONALSETTING_H
#define OPTIONALSETTING_H

#include <QWidget>

namespace Ui {
    class optionalSetting;
}

class OptionalSetting : public QWidget
{
    Q_OBJECT

public:
    explicit OptionalSetting(QWidget* parent = nullptr);
    ~OptionalSetting() override;

    void init(const QString& labelText, const QString& value, bool enabled);

signals:
    void checked(bool value);
    void valueChanged(const QString& value);

private:
    Ui::optionalSetting* ui;
};

#endif // OPTIONALSETTING_H
