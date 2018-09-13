#include <QPushButton>
#include <QDebug>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHeaderView>
#include "astreewidget.h"
#include "astreeitem.h"

AsTreeWidget::AsTreeWidget(QWidget* parent): QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &AsTreeWidget::customContextMenuRequested, this, &AsTreeWidget::showContextMenu);
    header()->setStretchLastSection(false);
}

void AsTreeWidget::update()
{
    for (AsTreeItem* treeItem : items)
    {
        treeItem->update();
    }
}

void AsTreeWidget::showContextMenu(const QPoint& point)
{
    AsTreeItem* item = (AsTreeItem*) itemAt(point);
    if (item != nullptr)
    {
        QMenu menu(this);
        ISyncItem* syncItem = item->syncItem();
        menu.addAction("Recheck", [=] ()
        {
            syncItem->check();
        });
        IRepository* repository = dynamic_cast<IRepository*>(syncItem);
        if(repository != nullptr)
        {
            menu.addAction("Force join", [=] ()
            {
                repository->join();
            });
            menu.addAction("Force start", [=] ()
            {
                repository->start();
            });
        }
        menu.exec(QCursor::pos());
    }
}

void AsTreeWidget::setRepositories(QList<IRepository*> repositories)
{
    clear();
    items.clear();
    addRepositories(repositories);
    header()->resizeSections(QHeaderView::ResizeToContents);
    header()->resizeSection(1, header()->sectionSize(1) + 50); //Name
    header()->resizeSection(4, header()->sectionSize(4) + 25); //Size
}

void AsTreeWidget::addRepositories(QList<IRepository*> repositories)
{
    for (IRepository* repo : repositories)
    {
        QCheckBox* repoCheckBox = new QCheckBox(this);

        QPushButton* startButton = new QPushButton("Start Game", this);
        QPushButton* joinButton = new QPushButton("Join Server", this);
        AsTreeItem* repoItem = new AsTreeItem(this, repo, repoCheckBox, startButton, joinButton);
        addTopLevelItem(repoItem);

        connect(startButton, &QPushButton::pressed, [=] () {
            repo->start();
        });
        connect(joinButton, &QPushButton::pressed, [=] () {
            repo->join();
        });
        connect(this, &AsTreeWidget::itemClicked, [=] (QTreeWidgetItem* item, int) {
            if (item->isExpanded())
            {
                collapseItem(item);
                return;
            }
            expandItem(item);
        });

        setItemWidget(repoItem, 0, repoCheckBox);
        setItemWidget(repoItem, 5, startButton);
        setItemWidget(repoItem, 6, joinButton);
        items.append(repoItem);
        for (ISyncItem* mod : repo->uiMods())
        {
            QCheckBox* modCheckBox = new QCheckBox(this);
            AsTreeItem* modItem = new AsTreeItem(repoItem, mod, modCheckBox);
            setItemWidget(modItem, 0, modCheckBox);
            repoItem->addChild(modItem);
            items.append(modItem);
        }
    }
}
