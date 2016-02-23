/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include "debug.h"
#include <QStringList>
#include "treeitem.h"

TreeItem::TreeItem(const QString& name, TreeItem* parentItem)//:
    //QObject(0),
    //m_parentItem(parentItem),
    //name_(name)
{
    m_parentItem = parentItem;
    name_ = name;
    DBG << "name set perf.gap BEGIN name=" << name;
}

void TreeItem::appendChild(TreeItem *item)
{
    m_childItems.append(item);
}

TreeItem* TreeItem::child(int row)
{
    return m_childItems.value(row);
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

int TreeItem::columnCount() const
{
    //TODO: Print amount of roles
    return 6;
}

TreeItem* TreeItem::parentItem()
{
    return m_parentItem;
}

QList<TreeItem*> TreeItem::childItems() const
{
    return m_childItems;
}

void TreeItem::setParentItem(TreeItem *parentItem)
{
    m_parentItem = parentItem;
}

int TreeItem::row() const
{
    int rVal = 0;
    if (m_parentItem)
    {
        rVal = m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));
    }

    return rVal;
}
