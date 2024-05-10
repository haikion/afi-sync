#include <QMetaObject>

#include "afisynclogger.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

using namespace Qt::StringLiterals;

ModAdapter::ModAdapter(const QSharedPointer<Mod>& mod, Repository* repo, bool isOptional, int index):
    SyncItem(mod->name()),
    mod_(mod),
    key_(mod->key()),
    repo_(repo),
    isOptional_(isOptional)
{
    setFileSize(mod->fileSize());
    repo->appendModAdapter(QSharedPointer<ModAdapter>(this), index);
    QObject::connect(mod.get(), &Mod::fileSizeInitialized, repo, [=] (auto fileSize)
    {
        setFileSize(fileSize);
        repo->setFileSize(repo->fileSize() + fileSize);
        emit fileSizeInitialized();
    });
    QMetaObject::invokeMethod(mod.get(), [=] ()
    {
        mod->appendModAdapter(this);
    }, Qt::QueuedConnection);
}

ModAdapter::~ModAdapter()
{
    QMetaObject::invokeMethod(mod_.get(), [=] ()
    {
        mod_->removeModAdapter(this);
    }, Qt::BlockingQueuedConnection);
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
    {
        return true;
    }

    return settings_.ticked(name(), repo()->name());
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

    settings_.setTicked(name(), repo()->name(), checked);
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

QSharedPointer<Mod> ModAdapter::mod() const
{
    return mod_;
}

Repository* ModAdapter::repo() const
{
    return repo_;
}

void ModAdapter::forceCheck() const
{
    QMetaObject::invokeMethod(mod_.get(), &Mod::forceCheck);
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
