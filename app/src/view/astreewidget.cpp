#include <utility>

#include <QPushButton>
#include <QDebug>
#include <QContextMenuEvent>
#include <QMenu>
#include <QHeaderView>
#include "../model/modadapter.h"
#include "astreewidget.h"
#include "astreeitem.h"

AsTreeWidget::AsTreeWidget(QWidget* parent): QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setExpandsOnDoubleClick(false);
    header()->setStretchLastSection(false);

    connect(this, &AsTreeWidget::customContextMenuRequested, this, &AsTreeWidget::showContextMenu);
    connect(this, &AsTreeWidget::itemClicked, [=] (QTreeWidgetItem* item, int) {
        if (item->isExpanded())
        {
            collapseItem(item);
            return;
        }
        expandItem(item);
    });
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
    AsTreeItem* item = dynamic_cast<AsTreeItem*>(itemAt(point));
    if (item != nullptr)
    {
        QMenu menu(this);
        ISyncItem* syncItem = item->syncItem();
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
            menu.addAction("Recheck", [=] ()
            {
                syncItem->check();
            });
        }
        else
        {
            const ModAdapter* modAdapter = dynamic_cast<ModAdapter*>(syncItem);
            menu.addAction("Recheck", [=] () {
                modAdapter->forceCheck();
            });
        }
        menu.exec(QCursor::pos());
    }
}

void AsTreeWidget::setRepositories(QList<IRepository*> repositories)
{
    clear();
    items.clear();
    addRepositories(std::move(repositories));
    header()->resizeSections(QHeaderView::ResizeToContents);
    header()->resizeSection(2, header()->sectionSize(2) + 50); //Status
    header()->resizeSection(4, header()->sectionSize(4) + 25); //Size
    //Hack to make name column take remaining space
    header()->setSectionResizeMode(1, QHeaderView::Stretch);
    header()->sectionSize(1); //This is required!
    header()->setSectionResizeMode(1, QHeaderView::Interactive);
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
