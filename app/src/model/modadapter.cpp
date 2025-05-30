#include "modadapter.h"

#include <QDir>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSharedPointer>

#include "afisynclogger.h"
#include "mod.h"
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
    QObject::connect(mod.get(), &Mod::fileSizeInitialized, repo, [=, this] (auto fileSize)
    {
        setFileSize(fileSize);
        repo->setFileSize(repo->fileSize() + fileSize);
        emit fileSizeInitialized();
    });
    QMetaObject::invokeMethod(mod.get(), [=, this] ()
    {
        mod->appendModAdapter(this);
    }, Qt::QueuedConnection);
}

ModAdapter::~ModAdapter()
{
    QMetaObject::invokeMethod(mod_.get(), [this] ()
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

QString ModAdapter::path() const
{
    QString baseDir = SettingsModel::instance().modDownloadPath();
    QString nameVar = name();
    // Check for invalid characters (e.g., path traversal sequences) in name
    static QRegularExpression invalidChars(R"([<>:"\/|?*]|(\.\.))");
    if (nameVar.contains(invalidChars)) {
        LOG_WARNING << "Invalid characters in name: " << nameVar;
        return {};
    }
    QDir dir(baseDir + '/' + nameVar);
    return dir.exists() ? dir.path() : QString();
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
