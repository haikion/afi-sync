/*
    treeitem.cpp

    A container for items of data supplied by the simple tree model.
*/

#include <QStringList>
#include "afisynclogger.h"
#include "treeitem.h"

TreeItem::TreeItem(const QString& name, TreeItem* parentItem)
{
    m_parentItem = parentItem;
    name_ = name;
}

void TreeItem::appendChild(TreeItem* item, int index)
{
    m_childItems.insert(index, item);
}

bool TreeItem::removeChild(TreeItem* child)
{
    return m_childItems.removeAll(child) > 0;
}

TreeItem* TreeItem::child(int row)
{
    return m_childItems.value(row);
}

int TreeItem::childCount() const
{
    return m_childItems.count();
}

TreeItem* TreeItem::parentItem()
{
    return m_parentItem;
}

QList<TreeItem*> TreeItem::childItems() const
{
    return m_childItems;
}

void TreeItem::setParentItem(TreeItem* parentItem)
{
    m_parentItem = parentItem;
}

int TreeItem::row() const
{
    int rVal = -1;
    if (m_parentItem)
    {
        QList<TreeItem*> childs = m_parentItem->m_childItems;
        if (childs.contains(const_cast<TreeItem*>(this)))
        {
            rVal = m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));
        }
    }
    if (rVal == -1)
    {
        LOG_ERROR << name_ << " has no row.";
    }

    return rVal;
}

QVariant TreeItem::data(int column) const
{
    //TODO: Figure out why this is called.
    LOG << "column = " << column << " name = " << name_;
    return QVariant();
}
