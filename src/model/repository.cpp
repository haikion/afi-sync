#include <algorithm>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QDateTime>
#include "debug.h"
#include "settingsmodel.h"
#include "settingsmodel.h"
#include "repository.h"
#include "installer.h"
#include "modviewadapter.h"

Repository::Repository(const QString& name, const QString& serverAddress, unsigned port,
                      QString password, RootItem* parent):
    SyncItem(name, parent),
    sync_(parent->sync()),
    serverAddress_(serverAddress),
    port_(port),
    password_(password),
    ready_(true)
{
    DBG << "Created repo name =" << name;
}

void Repository::update()
{
    updateEtaAndStatus();
    updateView(this);
}

void Repository::stopUpdates()
{
    DBG << "stopped";
    for (Mod* mod : mods())
    {
        mod->stopUpdates();
    }
}

void Repository::startUpdates()
{
    //updateTimer_.start();
    for (Mod* mod : mods())
    {
        mod->startUpdates();
    }
}


Repository::~Repository()
{
    DBG << "name =" << name();
    for (Mod* mod : mods())
    {
        removeMod(mod->key());
    }
    RootItem* parent = parentItem();
    parent->removeChild(this);
    parent->layoutChanged();
}

void Repository::processCompletion()
{
    ready_ = true;
    for (Mod* mod : mods())
    {
        mod->processCompletion();
    }
    SettingsModel::setInstallDate(name(), QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
}

void Repository::updateView(TreeItem* item, int row)
{
    parentItem()->updateView(item, row);
}

void Repository::checkboxClicked(bool offline)
{
    SyncItem::checkboxClicked();
    //FIXME: Is this a crash risk?
    //updateTimer_.stop();
    setStatus("Processing new mods...");
    updateView(this);
    for (Mod* mod : mods())
    {
        QMetaObject::invokeMethod(mod, "repositoryEnableChanged",
                                  Qt::QueuedConnection, Q_ARG(bool, offline));
    }
    //updateTimer_.start();
    DBG << "checked()=" << checked();
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
        DBG << "Using params file";
        arguments << "-applaunch" << "107410" << "-par=" + paramsFile;
        executable = steamExecutable;
    }
    else if (modsParameter().size() > 0)
    {
        DBG << "Failsafe activated because parameter file path is incorrect. paramsFile =" << paramsFile;
        arguments << modsParameter();
    }
    arguments << "-noLauncher";
    if (SettingsModel::battlEyeEnabled())
    {
        arguments << "-useBe";
    }
    if (extraParams.size() > 0)
    {
        arguments << extraParams;
    }
    QString paramsString = SettingsModel::launchParameters();
    if (paramsString.size() > 0)
    {
        QStringList userParams = paramsString.split(" ");
        arguments << userParams;
    }
    DBG << " name() =" << name()
             << "executable =" << executable
             << "arguments =" << arguments;
    QProcess::startDetached(executable, arguments);
}

QString Repository::createParFile(const QString& parameters)
{
    QString fileName = "afiSyncParameters.txt";
    QString path = SettingsModel::arma3Path() + "/" + fileName;
    DBG << path;
    QFile file(path);
    file.remove();
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream fileStream(&file);
    fileStream << parameters;
    file.close();
    return fileName;
}

void Repository::updateEtaAndStatus()
{
    //Eta = max(active_folders) + sum(queued_folders)
    int maxEta = 0;
    int sumEta = 0;
    QSet<QString> modStatuses;
    for (Mod* item : mods())
    {
        if (item->status() == SyncStatus::QUEUED)
        {
            sumEta += item->eta();
        }
        else
        {
            maxEta = std::max(item->eta(), maxEta);
        }
        modStatuses.insert(item->status());
    }
    setEta(maxEta + sumEta);
    //Status
    QSet<QString> readyStatuses;
    readyStatuses.insert(SyncStatus::READY);
    readyStatuses.insert(SyncStatus::INACTIVE);
    readyStatuses.insert(SyncStatus::READY_PAUSED);
    if (!checked())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if ((modStatuses - readyStatuses).size() == 0)
    {
        //All mods are ready so repo is ready.
        setStatus(SyncStatus::READY);
    }
    else if (modStatuses.contains(SyncStatus::DOWNLOADING))
    {
        setStatus(SyncStatus::DOWNLOADING);
        ready_ = false;
    }
    else if (modStatuses.contains(SyncStatus::CHECKING))
    {
        setStatus(SyncStatus::CHECKING);
    }
    else
    {
        setStatus(SyncStatus::WAITING);
    }
}

QString Repository::modsParameter() const
{
    if (childItems().size() == 0)
    {
        return "";
    }
    QString rVal = "-mod=";
    for (const Mod* mod : mods())
    {
        if (mod->checked())
        {
            QDir modDir(SettingsModel::modDownloadPath() + "/" + mod->name());
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

void Repository::appendMod(Mod* item)
{
    item->addRepository(this);
    ModViewAdapter* adp = new ModViewAdapter(item, this);
    item->addModViewAdapter(adp);
    TreeItem::appendChild(adp);
}

QString Repository::startText()
{
    if (checked() == false)
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
    if (checked() == false)
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
    DBG;
    return sync_;
}

void Repository::enableMods()
{
    DBG << "name =" << name();
    for (Mod* mod : mods())
    {
        if (!mod->checked())
        {
            mod->checkboxClicked();
        }
    }
}

bool Repository::removeMod(const QString& key)
{
    for (Mod* mod : mods())
    {
        if (mod->key().toLower() == key.toLower())
        {
            DBG << "Removing mod" << mod->name() << "from repository" << name();
            removeMod(mod);
            return true;
        }
    }
    DBG << "ERROR: key" << key << "not found in repository" << name();
    return false;
}

bool Repository::removeMod(Mod* mod)
{
    //Removes mod view adapter.
    if (!mod->removeRepository(this))
    {
        return false;
    }
    parentItem()->layoutChanged();
    if (mod->repositories().size() == 0)
    {
        //Mod may not exist if it doesn't belong to any repo.
        sync_->removeFolder2(mod->key());
        delete mod;
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
        rVal.append(static_cast<ModViewAdapter*>(item)->mod());
    }
    return rVal;
}

RootItem* Repository::parentItem()
{
    return static_cast<RootItem*>(TreeItem::parentItem());
}
