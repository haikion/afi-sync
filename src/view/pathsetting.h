#ifndef PATHSETTING_H
#define PATHSETTING_H

#include <QWidget>
#include <QString>
#include <QFileDialog>

namespace Ui {
    class PathSetting;
}

class PathSetting : public QWidget
{
    Q_OBJECT

public:
    explicit PathSetting(QWidget *parent = 0);
    ~PathSetting();

    void init(const QString& labelText, const QString& value);

signals:
    void textEdited(const QString& newValue);
    void resetPressed();

private slots:
    void on_browseButton_pressed();

private:
    Ui::PathSetting *ui;
    QFileDialog fileDialog;
};

#endif // PATHSETTING_H