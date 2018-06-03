#include <algorithm>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include "afisynclogger.h"
#include "fileutils.h"
#include "installer.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"
#include "settingsmodel.h"

Repository::Repository(const QString& name, const QString& serverAddress, unsigned port,
                      QString password, RootItem* parent):
    SyncItem(name, parent),
    sync_(parent->sync()),
    serverAddress_(serverAddress),
    port_(port),
    password_(password),
    ready_(true),
    battlEyeEnabled_(true)
{
    LOG << "Created repo name = " << name;
    update();
    if (ticked())
        startUpdates();
}

void Repository::update()
{
    updateEtaAndStatus();
    updateView(this);
}

void Repository::stopUpdates()
{
    LOG;
    for (Mod* mod : mods())
    {
        mod->stopUpdates();
    }
}

void Repository::setBattlEyeEnabled(bool battleEyeEnabled)
{
    battlEyeEnabled_ = battleEyeEnabled;
}

bool Repository::ticked() const
{
    return SettingsModel::ticked("", name());
}

void Repository::setTicked(bool ticked)
{
    SettingsModel::setTicked("", name(), ticked);
    update();
}

void Repository::startUpdates()
{
    LOG;
    for (Mod* mod : mods())
    {
        mod->startUpdates();
    }
}

Repository::~Repository()
{
    LOG << "name = " << name();
    for (Mod* mod : mods())
    {
        //Remove mod object from repository but keep
        //mod settings.
        removeMod(mod, false);
    }
    RootItem* parent = parentItem();
    parent->removeChild(this);
    parent->rowsChanged();
}

void Repository::check()
{
    for (SyncItem* mod : mods())
    {
        mod->check();
    }
}

void Repository::processCompletion()
{
    ready_ = true;
    for (Mod* mod : mods())
    {
        if (mod->ticked())
            mod->processCompletion();
    }
    SettingsModel::setInstallDate(name(), QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
}

void Repository::updateView(TreeItem* item, int row)
{
    parentItem()->updateView(item, row);
}

void Repository::changed(bool offline)
{
    for (Mod* mod : mods())
    {
        mod->repositoryChanged(offline);
    }
}

void Repository::checkboxClicked(bool offline)
{
    stopUpdates();
    setTicked(!ticked());
    setStatus("Processing new mods...");
    update();
    changed(offline);
    if (ticked())
        startUpdates();
}

void Repository::checkboxClicked()
{
    checkboxClicked(false);
}

void Repository::join()
{
    generalLaunch(joinParameters());
}

void Repository::launch()
{
    generalLaunch();
}

void Repository::generalLaunch(const QStringList& extraParams)
{
    QString executable = SettingsModel::arma3Path() + "\\arma3launcher.exe";
    QString steamExecutable = SettingsModel::steamPath() + "\\Steam.exe";
    QStringList arguments;

    //arma3launcher wont start the game with correct parameters
    //if steam isn't running.
    //Create tmp file containing parameters because steam.exe can only pass
    //~1000 characters in parameters
    QString paramsFile = createParFile(modsParameter());
    if (QFileInfo(steamExecutable).exists())
    {
        LOG << "Using params file";
        arguments << "-applaunch" << "107410" << "-par=" + paramsFile;
        executable = steamExecutable;
    }
    else if (modsParameter().size() > 0)
    {
        LOG << "Failsafe activated because parameter file path is incorrect. paramsFile = " << paramsFile;
        arguments << modsParameter();
    }
    arguments << "-noLauncher";
    if (battlEyeEnabled_)
        arguments << "-useBe";

    if (extraParams.size() > 0)
        arguments << extraParams;

    QString paramsString = SettingsModel::launchParameters();
    if (paramsString.size() > 0)
    {
        QStringList userParams = paramsString.split(" ");
        arguments << userParams;
    }
    LOG << " name() = " << name()
        << " executable = " << executable
        << " arguments = " << arguments;
    QProcess::startDetached(executable, arguments);
}

QString Repository::createParFile(const QString& parameters)
{
    static const QString FILE_NAME = "afiSyncParameters.txt";
    const QString path = SettingsModel::arma3Path() + "/" + FILE_NAME;
    LOG << path;
    QFile file(path);
    FileUtils::safeRemove(file);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream fileStream(&file);
    fileStream.setCodec("UTF-8");
    fileStream << parameters;
    file.close();
    return FILE_NAME;
}

//Which mod takes longest to complete?
int Repository::calculateEta() const
{
    int maxEta = 0;

    for (Mod* item : mods())
    {
        maxEta = std::max(item->eta(), maxEta);
    }
    return maxEta;
}

void Repository::updateEtaAndStatus()
{
    QSet<QString> modStatuses;

    for (const Mod* item : mods())
    {
        modStatuses.insert(item->status());
    }
    //Status
    QSet<QString> readyStatuses;
    readyStatuses.insert(SyncStatus::READY);
    readyStatuses.insert(SyncStatus::INACTIVE);
    readyStatuses.insert(SyncStatus::READY_PAUSED);
    if (!ticked())
    {
        setStatus(SyncStatus::INACTIVE);
        //If repository is not activated then
        //set eta to 00:00:00
        setEta(0);
    }
    else if ((modStatuses - readyStatuses).size() == 0)
    {
        //All mods are ready so repo is ready.
        setStatus(SyncStatus::READY);
        setEta(0);
    }
    else if (modStatuses.contains(SyncStatus::DOWNLOADING) || modStatuses.contains(SyncStatus::DOWNLOADING_PATCHES))
    {
        setStatus(SyncStatus::DOWNLOADING);
        setEta(calculateEta());
        ready_ = false;
    }
    else if (modStatuses.contains(SyncStatus::PATCHING))
    {
        setStatus(SyncStatus::PATCHING);
        setEta(calculateEta());
    }
    else if (modStatuses.contains(SyncStatus::CHECKING))
    {
        setStatus(SyncStatus::CHECKING);
        setEta(calculateEta());
    }
    else
    {
        setStatus(SyncStatus::WAITING);
        setEta(calculateEta());
    }
}

QString Repository::modsParameter() const
{
    if (childItems().size() == 0)
    {
        return "";
    }
    QString rVal = "-mod=";
    for (const ModAdapter* modAdapter : modAdapters())
    {
        if (modAdapter->ticked())
        {
            QDir modDir(SettingsModel::modDownloadPath() + "/" + modAdapter->name());
            rVal += modDir.absolutePath() + ";";
        }
    }
    rVal.remove(rVal.length() - 1, 1);
    return rVal;
}

QStringList Repository::joinParameters() const
{
    QStringList rVal;
    rVal << "-connect=" + serverAddress_
         << "-port=" + QString::number(port_);
    if (password_ != "")
    {
        rVal << "-password=" + password_;
    }
    return rVal;
}

void Repository::appendModAdapter(ModAdapter* adp, int index)
{
    setFileSize(fileSize() + adp->mod()->fileSize());

    TreeItem::appendChild(adp, index);
}

QString Repository::startText()
{
    if (ticked() == false)
    {
        return "Start Disabled";
    }
    if (status() == SyncStatus::READY)
    {
        return "Start";
    }
    return "Start Disabled";
}

QString Repository::joinText()
{
    if (ticked() == false)
    {
        return "Join Disabled";
    }
    if (status() == SyncStatus::READY)
    {
        return "Join";
    }
    return "Join Disabled";
}

ISync* Repository::sync() const
{
    return sync_;
}

void Repository::enableMods()
{
    LOG << "name = " << name();
    for (ModAdapter* adp : modAdapters())
    {
        if (adp->isOptional() && !adp->ticked())
        {
            adp->checkboxClicked();
        }
    }
}

bool Repository::removeMod(const QString& key)
{
    for (Mod* mod : mods())
    {
        if (mod->key() == key)
        {
            LOG << "Removing mod " << mod->name() << " from repository " << name();
            removeMod(mod);
            return true;
        }
    }
    LOG << "Key " << key << " not found in repository " << name();
    return false;
}

bool Repository::removeMod(Mod* mod, bool removeFromSync)
{
    //Removes mod view adapter.
    if (!mod->removeRepository(this))
    {
        LOG_ERROR << "Unable to remove " << mod->name() << " from repository " << name();
        return false;
    }
    parentItem()->rowsChanged();
    if (mod->repositories().size() == 0 && removeFromSync)
    {
        //Mod may not exist if it doesn't belong to any repo.
        sync_->removeFolder(mod->key());
        /* Deleting a QObject while pending events are waiting to be delivered can cause a crash.
         * You must not delete the QObject directly if it exists in a different thread than the
         * one currently executing. Use deleteLater() instead, which will cause the event loop
         * to delete the object after all pending events have been delivered to it.
         */
        mod->deleteLater(); //Mod runs in worker thread.
    }
    return true;
}

bool Repository::contains(const QString& key) const
{
    for (const Mod* mod : mods())
    {
        if (mod->key() == key)
            return true;
    }
    return false;
}

QList<Mod*> Repository::mods() const
{
    QList<Mod*> rVal;
    for (TreeItem* item : TreeItem::childItems())
    {
        rVal.append(static_cast<ModAdapter*>(item)->mod());
    }
    return rVal;
}

QList<ModAdapter*> Repository::modAdapters() const
{
    QList<ModAdapter*> rVal;
    for (TreeItem* item : TreeItem::childItems())
    {
        rVal.append(static_cast<ModAdapter*>(item));
    }
    return rVal;
}


RootItem* Repository::parentItem()
{
    return static_cast<RootItem*>(TreeItem::parentItem());
}
