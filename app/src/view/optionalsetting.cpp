#include <QIntValidator>
#include "optionalsetting.h"
#include "ui_optionalsetting.h"

OptionalSetting::OptionalSetting(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::optionalSetting)
{
    ui->setupUi(this);
    ui->lineEdit->setValidator(new QIntValidator(this));
}

OptionalSetting::~OptionalSetting()
{
    delete ui;
}

void OptionalSetting::init(const QString& labelText, const QString& value, const bool enabled)
{
    ui->checkBox->setText(labelText);
    ui->lineEdit->setText(value);
    connect(ui->checkBox, &QCheckBox::clicked, this, &OptionalSetting::checked);
    connect(ui->lineEdit, &QLineEdit::editingFinished, this, [this]
    {
        emit valueChanged(ui->lineEdit->text());
    });
    ui->checkBox->setChecked(enabled);
}
