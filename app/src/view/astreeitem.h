#ifndef ASTREEITEM_H
#define ASTREEITEM_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QCheckBox>
#include <QPushButton>
#include <QObject>

#include "../model/interfaces/isyncitem.h"

class AsTreeItem : public QTreeWidgetItem
{
public:
    explicit AsTreeItem(QTreeWidgetItem* parent, ISyncItem* syncItem, QCheckBox* checkBox);
    explicit AsTreeItem(QTreeWidget* parent, ISyncItem* syncItem, QCheckBox* checkBox,
                            QPushButton* startButton, QPushButton* joinButton);

    void update();
    ISyncItem* syncItem();

private:
    ISyncItem* syncItem_{nullptr};
    QCheckBox* checkBox_{nullptr};
    bool wantedCheckBoxState_{false};
    QPushButton* startButton_{nullptr};
    QPushButton* joinButton_{nullptr};
    // Keep this last
    QObject context_;

    void init();
    void initCheckBox();
    void updateCheckBox();
    void updateButtons();
};

#endif // ASTREEITEM_H
