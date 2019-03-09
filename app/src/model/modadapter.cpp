#include "afisynclogger.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

ModAdapter::ModAdapter(Mod* mod, Repository* repo, bool isOptional, int index):
    SyncItem(mod->name()),
    mod_(mod),
    repo_(repo),
    isOptional_(isOptional)
{
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

QString ModAdapter::statusStr()
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

bool ModAdapter::ticked()
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

bool ModAdapter::optional()
{
    return isOptional_;
}

QString ModAdapter::progressStr()
{
    return mod_->progressStr();
}

Mod* ModAdapter::mod() const
{
    return mod_;
}

Repository* ModAdapter::repo() const
{
    return repo_;
}

void ModAdapter::forceCheck() const
{
    mod_->forceCheck();
}

bool ModAdapter::selected()
{
    return ticked() && repo_->ticked();
}

QString ModAdapter::key() const
{
    return mod_->key();
}
