#include "debug.h"
#include "modviewadapter.h"

ModViewAdapter::ModViewAdapter(Mod* mod, Repository* repo):
    SyncItem(mod->name(), repo),
    mod_(mod),
    repo_(repo)
{
    setParentItem(repo);
    timer_.setInterval(1000);
    connect(&timer_, SIGNAL(timeout()), this, SLOT(updateView()));
    timer_.start();
}

QString ModViewAdapter::checkText()
{
    return mod_->checkText();
}

QString ModViewAdapter::startText()
{
    return mod_->startText();
}

QString ModViewAdapter::joinText()
{
    return mod_->joinText();
}

QString ModViewAdapter::status() const
{
    return mod_->status();
}

void ModViewAdapter::checkboxClicked()
{
    mod_->checkboxClicked();
}

Mod* ModViewAdapter::mod() const
{
    return mod_;
}

void ModViewAdapter::updateView()
{
    setEta(mod_->eta());
    repo_->updateView(this, repo_->childItems().indexOf(this));
}
