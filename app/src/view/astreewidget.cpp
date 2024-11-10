#include "astreewidget.h"

#include <QDesktopServices>
#include <QHeaderView>
#include <QMenu>
#include <QPushButton>
#include <QUrl>

#include "../model/modadapter.h"
#include "astreeitem.h"

using namespace Qt::StringLiterals;

AsTreeWidget::AsTreeWidget(QWidget* parent): QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setExpandsOnDoubleClick(false);
    header()->setStretchLastSection(false);

    connect(this, &AsTreeWidget::customContextMenuRequested, this, &AsTreeWidget::showContextMenu);
    connect(this, &AsTreeWidget::itemClicked, [=] (QTreeWidgetItem* item, int)
    {
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
    for (AsTreeItem* treeItem : items_)
    {
        treeItem->update();
    }
}

void AsTreeWidget::showContextMenu(QPoint point)
{
    auto item = dynamic_cast<AsTreeItem*>(itemAt(point));
    if (item != nullptr)
    {
        QMenu menu(this);
        ISyncItem* syncItem = item->syncItem();
        auto repository = dynamic_cast<IRepository*>(syncItem);
        if(repository != nullptr)
        {
            menu.addAction(u"Force join"_s, [=] { repository->join(); });
            menu.addAction(u"Force start"_s, [=] { repository->start(); });
            menu.addAction(u"Recheck"_s, [=] { syncItem->check(); });
        }
        else
        {
            const ModAdapter* modAdapter = dynamic_cast<ModAdapter*>(syncItem);
            const QString path = modAdapter->path();
            if (!path.isEmpty())
            {
                menu.addAction(u"Recheck"_s, [=]() { modAdapter->forceCheck(); });
                menu.addAction(u"Show in file explorer"_s, [=] () {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
                });
            }
        }
        menu.exec(QCursor::pos());
    }
}

void AsTreeWidget::setRepositories(const QList<IRepository*>& repositories)
{
    clear();
    items_.clear();
    addRepositories(repositories);
    header()->resizeSections(QHeaderView::ResizeToContents);
    header()->resizeSection(2, header()->sectionSize(2) + 50); //Status
    header()->resizeSection(4, header()->sectionSize(4) + 25); //Size
    //Hack to make name column take remaining space
    header()->setSectionResizeMode(1, QHeaderView::Stretch);
    header()->sectionSize(1); //This is required!
    header()->setSectionResizeMode(1, QHeaderView::Interactive);
}

void AsTreeWidget::addRepositories(const QList<IRepository*>& repositories)
{
    for (IRepository* repo : repositories)
    {
        auto repoCheckBox = new QCheckBox(this);

        auto startButton = new QPushButton(u"Start Game"_s, this);
        auto joinButton = new QPushButton(u"Join Server"_s, this);
        auto repoItem = new AsTreeItem(this, repo, repoCheckBox, startButton, joinButton);
        addTopLevelItem(repoItem);

        connect(startButton, &QPushButton::pressed, [=] ()
        {
            repo->start();
        });
        connect(joinButton, &QPushButton::pressed, [=] ()
        {
            repo->join();
        });

        setItemWidget(repoItem, 0, repoCheckBox);
        setItemWidget(repoItem, 5, startButton);
        setItemWidget(repoItem, 6, joinButton);
        items_.append(repoItem);
        for (const auto& mod : repo->modAdapters())
        {
            auto modCheckBox = new QCheckBox(this);
            auto modItem = new AsTreeItem(repoItem, mod.get(), modCheckBox);
            setItemWidget(modItem, 0, modCheckBox);
            repoItem->addChild(modItem);
            items_.append(modItem);
        }
    }
}
