#ifndef TREEITEM_H
#define TREEITEM_H
#include <QVariant>
#include <QList>
#include "apis/btsync/btsapi2.h"

//Impolements Qt tree model item

class TreeItem
{
public:
    //explicit TreeItem(const QList<QVariant> &data, TreeItem *parentItem = 0);
    explicit TreeItem(const QString& name, TreeItem* parentItem = 0);
    ~TreeItem() = default;

    virtual void appendChild(TreeItem* child);
    TreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    int row() const;
    TreeItem *parentItem();
    void setParentItem(TreeItem *parentItem);

protected:
    QList<TreeItem*> childItems() const;

private:
    TreeItem* m_parentItem;
    QList<TreeItem*> m_childItems;
    QString name_;
};

#endif // TREEITEM_H
