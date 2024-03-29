#include <algorithm>
#include <QDateTime>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include "afisynclogger.h"
#include "fileutils.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"
#include "settingsmodel.h"

Repository::Repository(const QString& name, const QString& serverAddress, unsigned port, const QString& password):
    SyncItem(name),
    serverAddress_(serverAddress),
    port_(port),
    password_(password),
    battlEyeEnabled_(true)
{
    LOG << "Created repo name = " << name;
    if (ticked()) {
        setStatus(SyncStatus::WAITING);
        activeTimer_.start();
        startUpdates();
    }
    update();
}

void Repository::stopUpdates()
{
    for (ModAdapter* modAdapter : modAdapters_)
    {
        modAdapter->stopUpdates();
    }
}

void Repository::setBattlEyeEnabled(bool battleEyeEnabled)
{
    battlEyeEnabled_ = battleEyeEnabled;
}

bool Repository::ticked()
{
    return SettingsModel::ticked("", name());
}

void Repository::setTicked(bool ticked)
{
    SettingsModel::setTicked("", name(), ticked);
    update();
}

QString Repository::progressStr()
{
    if (!ticked())
        return "???";

    qint64 totalWanted = 0;
    qint64 totalWantedDone = 0;
    // Mods share the same delta patches torrent
    qint64 totalWantedDelta = 0;
    qint64 totalWantedDoneDelta = 0;
    for (Mod* mod : mods())
    {
        if (!mod->ticked())
            continue;

        if (mod->statusStr() == SyncStatus::CHECKING_PATCHES)
        {
            if (totalWantedDelta == 0)
            {
                totalWantedDelta = mod->totalWanted();
                totalWantedDoneDelta = mod->totalWantedDone();
            }
            continue;
        }

        totalWanted += mod->totalWanted();
        totalWantedDone += mod->totalWantedDone();
    }

    return Mod::toProgressStr(totalWanted + totalWantedDelta, totalWantedDone + totalWantedDoneDelta);
}

void Repository::setServerAddress(const QString& serverAddress)
{
    serverAddress_ = serverAddress;
}

void Repository::setPort(const unsigned& port)
{
    port_ = port;
}

void Repository::setPassword(const QString& password)
{
    password_ = password;
}

Repository* Repository::findRepoByName(const QString& name, QList<Repository*> repositories)
{
    for (Repository* repository : repositories)
    {
        if (repository->name() == name)
        {
            return repository;
        }
    }
    return nullptr;
}


void Repository::startUpdates()
{
    for (Mod* mod : mods())
    {
        mod->startUpdates();
    }
}

Repository::~Repository()
{
    LOG << "name = " << name();
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
    for (Mod* mod : mods())
    {
        if (mod->ticked())
            mod->processCompletion();
    }
    SettingsModel::setInstallDate(name(), QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
}

void Repository::changed()
{
    for (Mod* mod : mods())
    {
        QMetaObject::invokeMethod(mod, &Mod::repositoryChanged, Qt::QueuedConnection);
    }
}

void Repository::clearModAdapters()
{
    QList<ModAdapter*> modAdapters = modAdapters_;
    for (ModAdapter* modAdapter : modAdapters)
    {
        delete modAdapter;
        // Destructor removes from modAdapters_
    }
}

void Repository::checkboxClicked()
{
    SettingsModel::setTicked("", name(), !ticked());
    if (ticked()) {
        setStatus(SyncStatus::WAITING);
        activeTimer_.start();
        startUpdates();
    } else {
        update();
    }
    changed();
}

void Repository::join()
{
    generalLaunch(joinParameters());
}

void Repository::start()
{
    generalLaunch();
}

bool Repository::optional()
{
    return true; //Repositories are always optional
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
    if (QFileInfo::exists(steamExecutable))
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

    if (!extraParams.isEmpty())
        arguments << extraParams;

    QString paramsString = SettingsModel::launchParameters();
    if (paramsString.size() > 0)
    {
        QStringList userParams = paramsString.split(" ");
        arguments << userParams;
    }
    LOG << "name() = " << name()
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

QSet<QString> Repository::createReadyStatuses()
{
    QSet<QString> readyStatuses;
    readyStatuses.insert(SyncStatus::READY);
    readyStatuses.insert(SyncStatus::INACTIVE);
    readyStatuses.insert(SyncStatus::READY_PAUSED);
    return readyStatuses;
}

void Repository::removeAdapter(const Mod* mod)
{
    QList<ModAdapter*> adapters = modAdapters_;
    for (ModAdapter* modAdapter : adapters)
    {
        if (modAdapter->mod() == mod)
        {
            removeModAdapter(modAdapter);
        }
    }
}

void Repository::removeAdapter(const QString& key)
{
    QList<ModAdapter*> modAdapters = modAdapters_;
    for (ModAdapter* modAdapter : modAdapters)
    {
        if (modAdapter->key() == key)
        {
            delete modAdapter;
        }
    }
}

void Repository::removeModAdapter(ModAdapter* modAdapter)
{
    modAdapters_.removeAll(modAdapter);
    if (modAdapters_.isEmpty())
    {
        delete this;
    }
}

// TODO: Create static mathematic function that returns Repository status
// and takes in modStatuses
void Repository::update()
{
    static QSet<QString> readyStatuses = createReadyStatuses();

    if (activeTimer_.elapsed() < 2000 && ticked()) {
        // Give mods time to adjust
        return;
    }

    QSet<QString> modStatuses;
    for (Mod* item : mods())
    {
        modStatuses.insert(item->statusStr());
    }

    if (!ticked())
    {
        setStatus(SyncStatus::INACTIVE);
    }
    else if ((modStatuses - readyStatuses).isEmpty())
    {
        //All mods are ready so repo is ready.
        setStatus(SyncStatus::READY);
    }
    else if (modStatuses.contains(SyncStatus::DOWNLOADING) || modStatuses.contains(SyncStatus::DOWNLOADING_PATCHES))
    {
        setStatus(SyncStatus::DOWNLOADING);
    }
    else if (modStatuses.contains(SyncStatus::PATCHING))
    {
        setStatus(SyncStatus::PATCHING);
    }
    else if (modStatuses.contains(SyncStatus::CHECKING) || modStatuses.contains(SyncStatus::CHECKING_PATCHES))
    {
        setStatus(SyncStatus::CHECKING);
    }
    else if (modStatuses.contains(SyncStatus::MOVING_FILES))
    {
        setStatus(SyncStatus::MOVING_FILES);
    }
    else
    {
        setStatus(SyncStatus::WAITING);
    }
}

QString Repository::modsParameter()
{
    if (modAdapters_.isEmpty())
    {
        return QString();
    }
    QString rVal = "-mod=";
    auto charCounter = rVal.size();
    for (ModAdapter* modAdapter : modAdapters())
    {
        if (modAdapter->ticked())
        {
            QDir modDir(SettingsModel::modDownloadPath() + "/" + modAdapter->name());
            if ((charCounter + modDir.absolutePath().size()) >= 4096) {
                rVal.remove(rVal.length() - 1, 1);
                rVal += "\n-mod=";
                charCounter = 5;
                LOG << "Mod parameter split because it exceeded 4096 character limit.";
            }
            rVal += modDir.absolutePath() + ";";
            charCounter += (modDir.absolutePath().size() + 1);
        }
    }
    rVal.remove(rVal.length() - 1, 1);
    return rVal;
}

QSet<QString> Repository::modKeys() const
{
    QSet<QString> rVal;
    for (Mod* mod : mods())
    {
        rVal.insert(mod->key());
    }

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
    modAdapters_.insert(index, adp);
}

void Repository::enableMods()
{
    LOG << "name = " << name();
    for (ModAdapter* adp : modAdapters())
    {
        if (adp->optional() && !adp->ticked())
        {
            adp->checkboxClicked();
        }
    }
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
    for (ModAdapter* item : modAdapters_)
    {
        rVal.append(item->mod());
    }
    return rVal;
}

void Repository::removeDeprecatedMods(const QSet<QString>& jsonMods)
{
    const QSet<QString> deprecatedMods = modKeys() - jsonMods;
    for (const QString& key : deprecatedMods)
    {
        removeAdapter(key);
    }
}

QList<IMod*> Repository::uiMods() const
{
    QList<IMod*> retVal;
    for (ModAdapter* item : modAdapters_)
    {
        retVal.append(item);
    }
    return retVal;
}

QList<ModAdapter*> Repository::modAdapters() const
{
    return modAdapters_;
}
