#ifndef ASTREEWIDGET_H
#define ASTREEWIDGET_H

#include <QTreeWidget>
#include <QList>
#include <QObject>
#include <QCheckBox>
#include "../model/interfaces/irepository.h"
#include "../model/interfaces/isyncitem.h"
#include "astreeitem.h"

class AsTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    explicit AsTreeWidget(QWidget* parent = 0);

    void setRepositories(QList<IRepository*> repositories);


public slots:
    void update();

private slots:
    void showContextMenu(const QPoint& point);

private:
    QList<AsTreeItem*> items;
    void addRepositories(QList<IRepository*> repositories);
    QCheckBox* createCheckBox(ISyncItem* syncItem);
};

#endif // ASTREEWIDGET_H
