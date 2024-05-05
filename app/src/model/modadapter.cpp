#include <QMetaObject>

#include "afisynclogger.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

using namespace Qt::StringLiterals;

ModAdapter::ModAdapter(Mod* mod, Repository* repo, bool isOptional, int index):
    SyncItem(mod->name()),
    mod_(mod),
    repo_(repo),
    isOptional_(isOptional),
    key_(mod->key())
{
    //Connect everything
    setFileSize(mod->fileSize());
    repo->appendModAdapter(this, index);
    mod->appendModAdapter(this);
}

ModAdapter::~ModAdapter()
{
    repo_->removeModAdapter(this);
    QMetaObject::invokeMethod(mod_, "removeModAdapter", Qt::BlockingQueuedConnection, Q_ARG(ModAdapter*, this));
}

void ModAdapter::check()
{
    mod_->check();
}

QString ModAdapter::statusStr()
{
    return mod_->statusStr();
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
    QMetaObject::invokeMethod(mod_, &Mod::forceCheck);
}

bool ModAdapter::selected()
{
    return ticked() && repo_->ticked();
}

QString ModAdapter::key() const
{
    return key_;
}

void ModAdapter::stopUpdates()
{
    mod_->stopUpdates();
}
