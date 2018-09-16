#include "afisynclogger.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

ModAdapter::ModAdapter(Mod* mod, Repository* repo, bool isOptional, int index):
    SyncItem(mod->name(), repo),
    mod_(mod),
    repo_(repo),
    isOptional_(isOptional)
{
    setParentItem(repo);
    QString repoStr = repo_->name().replace("/| ","_");
    //Connect everything
    setFileSize(mod->fileSize());
    repo->appendModAdapter(this, index);
    mod->appendModAdapter(this);
    mod->appendRepository(repo);
}

void ModAdapter::check()
{
    mod_->check();
}

QString ModAdapter::checkText()
{
    if (!optional())
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

QString ModAdapter::statusStr() const
{
    return mod_->statusStr();
}

QString ModAdapter::etaStr() const
{
    return mod_->etaStr();
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

    return SettingsModel::ticked(name(), repo()->name());
}

void ModAdapter::processCompletion()
{
    mod_->processCompletion();
}

int ModAdapter::eta() const
{
    return mod_->eta();
}

void ModAdapter::setTicked(bool checked)
{
    if (!isOptional_)
    {
        LOG_ERROR << "Tried to set checked for non optional mod " << name();
        return;
    }

    SettingsModel::setTicked(name(), repo()->name(), checked);
    mod_->checkboxClicked();
}

bool ModAdapter::optional() const
{
    return isOptional_;
}

Mod* ModAdapter::mod() const
{
    return mod_;
}

Repository* ModAdapter::repo() const
{
    return repo_;
}
