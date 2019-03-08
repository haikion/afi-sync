#include "pathsetting.h"
#include "ui_pathsetting.h"

PathSetting::PathSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PathSetting)
{
    ui->setupUi(this);
    ui->lineEdit->setEnabled(false);

    connect(ui->resetButton, &QPushButton::pressed, this, &PathSetting::resetPressed);
    connect(&fileDialog, &QFileDialog::fileSelected, this, &PathSetting::setValueUser);
}

PathSetting::~PathSetting()
{
    delete ui;
}

void PathSetting::init(const QString& labelText, const QString& value)
{
    ui->label->setText(labelText);
    ui->lineEdit->setText(value);
}

void PathSetting::setValue(const QString& value)
{
    const QString nativeValue = QDir::toNativeSeparators(value);
    ui->lineEdit->setText(nativeValue);
}

void PathSetting::setValueUser(const QString& value)
{
    setValue(value);
    emit textEdited(ui->lineEdit->text());
}

void PathSetting::on_browseButton_pressed()
{
    fileDialog.setDirectory(ui->lineEdit->text());
    fileDialog.setFileMode(QFileDialog::Directory);
    // "QObject::connect: invalid null" parameter is normal here
    fileDialog.show();
}
