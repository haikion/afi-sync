#include "astreeitem.h"
#include "../model/interfaces/irepository.h"

AsTreeItem::AsTreeItem(QTreeWidget* parent, ISyncItem* syncItem, QCheckBox* checkBox,
                        QPushButton* startButton, QPushButton* joinButton):
    QTreeWidgetItem(parent),
    syncItem_(syncItem),
    checkBox_(checkBox),
    startButton_(startButton),
    joinButton_(joinButton)
{
    startButton_->setEnabled(false);
    joinButton_->setEnabled(false);
    init();
}

AsTreeItem::AsTreeItem(QTreeWidgetItem* parent, ISyncItem* syncItem, QCheckBox* checkBox):
    QTreeWidgetItem(parent),
    syncItem_(syncItem),
    checkBox_(checkBox)
{
    init();
}

void AsTreeItem::init()
{
    setText(1, syncItem_->name());
    setText(4, syncItem_->sizeStr());
    setFlags(Qt::ItemIsEnabled); //Disable selection
    initCheckBox();
    update();
}

void AsTreeItem::update()
{
    setText(2, syncItem_->statusStr());
    setText(3, syncItem_->progressStr());
    updateCheckBox();
    updateButtons();
}

// Updates checkbox state when syncitem
// state has been transitioned
void AsTreeItem::updateCheckBox()
{
    if (checkBox_->checkState() != Qt::PartiallyChecked)
    {
        return;
    }
    auto repository = dynamic_cast<IRepository*>(syncItem_);
    if (repository == nullptr)
    {
        //Mods don't wait for state transition because it depends on repository state
        // which would be too cubersome to monitor
        checkBox_->setCheckState(wantedCheckBoxState_ ? Qt::Checked : Qt::Unchecked);
        checkBox_->setEnabled(true);
        return;
    }

    if (wantedCheckBoxState_ && syncItem_->active())
    {
        checkBox_->setCheckState(Qt::Checked);
        checkBox_->setEnabled(true);
        return;
    }
    if (!wantedCheckBoxState_ && !syncItem_->active())
    {
        checkBox_->setCheckState(Qt::Unchecked);
        checkBox_->setEnabled(true);
        return;
    }
}

void AsTreeItem::updateButtons()
{
    if (startButton_ != nullptr && joinButton_ != nullptr)
    {
        const bool ready = syncItem_->statusStr() == SyncStatus::READY;
        startButton_->setEnabled(ready);
        joinButton_->setEnabled(ready);
    }
}

void AsTreeItem::initCheckBox()
{
    checkBox_->setEnabled(syncItem_->optional());
    checkBox_->setChecked(syncItem_->ticked());
    QObject::connect(checkBox_, &QCheckBox::clicked, &context_, [=] (bool value) {
        syncItem_->checkboxClicked();
        // New state is updated when sync item state updates
        wantedCheckBoxState_ = value;
        checkBox_->setCheckState(Qt::PartiallyChecked);
        checkBox_->setEnabled(false);
    });
}

ISyncItem*AsTreeItem::syncItem()
{
    return syncItem_;
}
