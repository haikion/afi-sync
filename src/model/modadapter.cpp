#include "debug.h"
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

    return SettingsModel::ticked(name(), repo()->name());
}

void ModAdapter::processCompletion()
{
    mod_->processCompletion();
}

void ModAdapter::setTicked(bool checked)
{
    if (!isOptional_)
    {
        DBG << "ERROR: Tried to set checked for non optional mod" << name();
        return;
    }

    SettingsModel::setTicked(name(), repo()->name(), checked);
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
    QString guiData = checkText() + progressText() + status();
    if (guiData == guiData_)
        return; //Avoid heavy UI updates when data has not been changed.

    repo_->updateView(this, repo_->childItems().indexOf(this));
    guiData_ = guiData;
}

Repository* ModAdapter::repo() const
{
    return repo_;
}
