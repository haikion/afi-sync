#include "debug.h"
#include "modadapter.h"
#include "repository.h"

ModAdapter::ModAdapter(Mod* mod, Repository* repo, bool isOptional, int index):
    SyncItem(mod->name(), repo),
    mod_(mod),
    repo_(repo),
    isOptional_(isOptional)
{
    setParentItem(repo);
    QString repoStr = repo_->name().replace("/| ","_");
    tickedKey_ = name() + "/" + repoStr + "ticked";
    //Connect everything
    repo->appendModAdapter(this, index);
    mod->appendModAdapter(this);
    mod->appendRepository(repo);
}

ModAdapter::~ModAdapter()
{
    //FIXME: Segfault
    //parentItem()->removeChild(this);
}

QString ModAdapter::checkText()
{
    if (!isOptional())
        return "disabled";

    QString rVal = ticked() ? "true" : "false";
    return rVal;
}

QString ModAdapter::startText()
{
    return mod_->startText();
}

QString ModAdapter::joinText()
{
    return mod_->joinText();
}

QString ModAdapter::status() const
{
    return mod_->status();
}

void ModAdapter::checkboxClicked()
{
    setTicked(!ticked());
    mod_->checkboxClicked();
}

bool ModAdapter::ticked() const
{
    if (!isOptional_)
        return true;

    return settings()->value(tickedKey_, false).toBool();
}

void ModAdapter::setTicked(bool checked)
{
    if (!isOptional_)
    {
        DBG << "ERROR: Tried to set checked for non optional mod" << name();
        return;
    }

    settings()->setValue(tickedKey_, checked);
    mod_->checkboxClicked();
}

bool ModAdapter::isOptional() const
{
    return isOptional_;
}

Mod* ModAdapter::mod() const
{
    return mod_;
}

void ModAdapter::updateView()
{
    setEta(mod_->eta());
    repo_->updateView(this, repo_->childItems().indexOf(this));
}
