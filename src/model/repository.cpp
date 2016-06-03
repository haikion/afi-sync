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
    updateTimer_.setInterval(1000);
    updateTimer_.start();
    connect(&updateTimer_, SIGNAL(timeout()), this, SLOT(update()));
}

void Repository::update()
{
    updateEtaAndStatus();
    updateView(this);
}

Repository::~Repository()
{
    DBG << "name =" << name();
    for (Mod* mod : mods())
    {
        mod->removeRepository(this);
        if (mod->repositories().size() == 0)
        {
            DBG << "Deleting mod: " << mod->name()
                     << "from repo: " << name();
            delete mod;
        }
    }
    for (TreeItem* modAdapter : childItems())
    {
        delete modAdapter;
    }
}

void Repository::processCompletion()
{
    ready_ = true;
    for (Mod* mod : mods())
    {
        mod->deleteExtraFiles();
        Installer::install(mod);
    }
    SettingsModel::setInstallDate(name(), QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
}

//FIXME: Clean up
//int Repository::lastModified()
//{
//    int lastModified = 0;
//    for (Mod* mod : mods())
//    {
//        lastModified = std::max(lastModified, mod->lastModified());
//    }
//    return lastModified;
//}

void Repository::updateView(TreeItem* item, int row)
{
    parentItem()->updateView(item, row);
}

void Repository::checkboxClicked(bool offline)
{
    SyncItem::checkboxClicked();
    updateTimer_.stop();
    setStatus("Processing new mods...");
    updateView(this);
    for (Mod* mod : mods())
    {
        QMetaObject::invokeMethod(mod, "repositoryEnableChanged",
                                  Qt::QueuedConnection, Q_ARG(bool, offline));
    }
    updateTimer_.start();
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

    //FIXME: Figure out what to do....
    //REMOVED FEATURE due to problems with BtSync reporting incorrect file listing...";
    //Failsafe: Check if installation needed incase download was not registered by AFISync
    //unsigned lastInstall = SettingsModel::installDate(name());
    //unsigned lastMod = lastModified();
    //DBG << "Checking if installation required lastModified =" << lastMod << "lastInstall =" << lastInstall;
    //if (lastMod > lastInstall)
    //{
    //    processCompletion();
    //}

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
        //executable = SettingsModel::arma3Path() + "/arma3battleye.exe";
        //arguments << "0" << "1"; //Be requires
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
    //QString path = QFileInfo("afiSyncParameters.txt").absoluteFilePath();
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
    //Eta
    int eta = 0;
    QSet<QString> modStatuses;
    for (Mod* item : mods())
    {
        eta = std::max(item->eta(), eta);
        modStatuses.insert(item->status());
    }
    setEta(eta);
    //Status
    QSet<QString> readyStatuses;
    readyStatuses.insert(SyncStatus::READY);
    readyStatuses.insert(SyncStatus::INACTIVE);
    if (!checked())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if ((modStatuses - readyStatuses).size() == 0)
    {
        //All mods are ready so repo is ready.
        setStatus(SyncStatus::READY);
        if (!ready_)
        {
            processCompletion(); //Runs only once.
        }
    }
    else if (modStatuses.contains(SyncStatus::DOWNLOADING))
    {
        setStatus(SyncStatus::DOWNLOADING);
        ready_ = false;
    }
    else if (modStatuses.contains(SyncStatus::INDEXING))
    {
        setStatus(SyncStatus::INDEXING);
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
    TreeItem::appendChild(new ModViewAdapter(item, this));
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
