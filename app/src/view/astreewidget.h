#ifndef ASTREEWIDGET_H
#define ASTREEWIDGET_H

#include <QCheckBox>
#include <QList>
#include <QObject>
#include <QTreeWidget>

#include "../model/interfaces/irepository.h"
#include "../model/interfaces/isyncitem.h"
#include "astreeitem.h"

class AsTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit AsTreeWidget(QWidget* parent = nullptr);

public slots:
    void update();
    void setRepositories(const QList<IRepository*>& repositories);

private slots:
    void showContextMenu(QPoint point);

private:
    QList<AsTreeItem*> items_;
    void addRepositories(const QList<IRepository*>& repositories);
    QCheckBox* createCheckBox(ISyncItem* syncItem);
};

#endif // ASTREEWIDGET_H
