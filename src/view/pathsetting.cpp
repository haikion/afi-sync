#include "pathsetting.h"
#include "ui_pathsetting.h"

PathSetting::PathSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PathSetting)
{
    ui->setupUi(this);
    ui->lineEdit->setEnabled(false);

    connect(ui->resetButton, SIGNAL(pressed()), this, SIGNAL(resetPressed()));
    connect(&fileDialog, SIGNAL(fileSelected(QString)), ui->lineEdit, SLOT(setText(QString)));
}

PathSetting::~PathSetting()
{
    delete ui;
}

void PathSetting::init(const QString& labelText, const QString& value)
{
    ui->label->setText(labelText);
    ui->lineEdit->setText(value);
    emit textEdited(value);
}

void PathSetting::on_browseButton_pressed()
{
    fileDialog.setDirectory(ui->lineEdit->text());
    fileDialog.show();
}
