#ifndef TREEITEM_H
#define TREEITEM_H
#include <limits>
#include <QVariant>
#include <QList>
#include "apis/isync.h"

//Impolements Qt tree model item

class TreeItem
{
public:
    explicit TreeItem(const QString& name, TreeItem* parentItem = 0);
    virtual ~TreeItem() = default;

    virtual void appendChild(TreeItem* child, int index = std::numeric_limits<int>::max());
    bool removeChild(TreeItem* child);
    TreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    int row() const;
    QVariant data(int column) const;
    TreeItem* parentItem();
    void setParentItem(TreeItem* parentItem);
    QList<TreeItem*> childItems() const;

private:
    TreeItem* m_parentItem;
    QList<TreeItem*> m_childItems;
    QString name_;
};

#endif // TREEITEM_H
