#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QTextStream>

#include "afisynclogger.h"
#include "fileutils.h"
#include "modadapter.h"
#include "repository.h"
#include "settingsmodel.h"

using namespace Qt::StringLiterals;

Repository::Repository(const QString& name, const QString& serverAddress, unsigned port, const QString& password):
    SyncItem(name),
    serverAddress_(serverAddress),
    port_(port),
    password_(password),
    battlEyeEnabled_(true)
{
    LOG << "Created repo with name " << name;
    if (SettingsModel::ticked({}, name)) {
        setStatus(SyncStatus::WAITING);
        activeTimer_.start();
        startUpdates();
    }
    update();
}

void Repository::stopUpdates()
{
    for (const auto& modAdapter : modAdapters_)
    {
        modAdapter->stopUpdates();
    }
}

void Repository::setBattlEyeEnabled(bool battleEyeEnabled)
{
    battlEyeEnabled_ = battleEyeEnabled;
}

bool Repository::isReady() const {
    for (const auto& modAdapter : modAdapters_)
    {
        if (modAdapter->statusStr() != SyncStatus::READY)
        {
            return false;
        }
    }
    return true;
}

bool Repository::ticked()
{
    return SettingsModel::ticked({}, name());
}

void Repository::setTicked(bool ticked)
{
    SettingsModel::setTicked({}, name(), ticked);
    update();
}

QString Repository::progressStr()
{
    if (!ticked())
    {
        return u"???"_s;
    }

    qint64 totalWanted = 0;
    qint64 totalWantedDone = 0;
    // Mods share the same delta patches torrent
    qint64 totalWantedDelta = 0;
    qint64 totalWantedDoneDelta = 0;
    for (const auto& mod : mods())
    {
        if (!mod->ticked())
        {
            continue;
        }

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

void Repository::setPort(unsigned port)
{
    port_ = port;
}

void Repository::setPassword(const QString& password)
{
    password_ = password;
}

QSharedPointer<Repository> Repository::findRepoByName(const QString& name, const QList<QSharedPointer<Repository>>& repositories)
{
    for (const auto& repository : repositories)
    {
        if (repository->name() == name)
        {
            return repository;
        }
    }
    return nullptr;
}

void Repository::startUpdates() const
{
    for (const auto& mod : mods())
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
    for (const auto& mod : mods())
    {
        mod->check();
    }
}

void Repository::processCompletion()
{
    for (const auto& mod : mods())
    {
        if (mod->ticked())
        {
            mod->processCompletion();
        }
    }
}

void Repository::changed() const
{
    for (const auto& mod: mods())
    {
        QMetaObject::invokeMethod(mod.get(), &Mod::repositoryChanged, Qt::QueuedConnection);
    }
}

void Repository::clearModAdapters()
{
    modAdapters_.clear();
}

void Repository::checkboxClicked()
{
    SettingsModel::setTicked({}, name(), !ticked());
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
        arguments << u"-applaunch"_s << u"107410"_s
                  << "-par=" + paramsFile;
        executable = steamExecutable;
    }
    else if (modsParameter().size() > 0)
    {
        LOG << "Failsafe activated because parameter file path is incorrect. paramsFile = " << paramsFile;
        arguments << modsParameter();
    }
    arguments << u"-noLauncher"_s;
    if (battlEyeEnabled_)
    {
        arguments << u"-useBe"_s;
    }

    if (!extraParams.isEmpty())
    {
        arguments << extraParams;
    }

    QString paramsString = SettingsModel::launchParameters();
    if (paramsString.size() > 0)
    {
        QStringList userParams = paramsString.split(u" "_s);
        arguments << userParams;
    }
    LOG << "name() = " << name()
        << " executable = " << executable
        << " arguments = " << arguments;
    QProcess::startDetached(executable, arguments);
}

QString Repository::createParFile(const QString& parameters)
{
    static const QString FILE_NAME = u"afiSyncParameters.txt"_s;
    const QString path = SettingsModel::arma3Path() + "/" + FILE_NAME;
    LOG << path;
    QFile file(path);
    FileUtils::safeRemove(file);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream fileStream(&file);
    fileStream.setEncoding(QStringConverter::Encoding::Utf8);
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

void Repository::removeAdapter(const QString& key)
{
    auto modAdapters = modAdapters_;
    for (const auto& modAdapter : modAdapters)
    {
        if (modAdapter->key() == key)
        {
            modAdapters_.removeOne(modAdapter);
            Q_ASSERT(!modAdapters_.contains(modAdapter));
        }
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
    for (const auto& item : mods())
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
    QString rVal = u"-mod="_s;
    auto charCounter = rVal.size();
    for (const auto& modAdapter : modAdapters_)
    {
        if (modAdapter->ticked())
        {
            QDir modDir(SettingsModel::modDownloadPath() + "/" + modAdapter->name());
            if ((charCounter + modDir.absolutePath().size()) >= 4096) {
                rVal.remove(rVal.length() - 1, 1);
                rVal += "\n-mod="_L1;
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

const QSet<QString> Repository::modKeys() const
{
    QSet<QString> rVal;
    for (const auto& mod : mods())
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
    if (!password_.isEmpty()) {
        rVal << "-password=" + password_;
    }
    return rVal;
}

void Repository::appendModAdapter(const QSharedPointer<ModAdapter>& adp, int index)
{
    setFileSize(fileSize() + adp->mod()->fileSize());
    modAdapters_.insert(index, adp);
}

bool Repository::contains(const QString& key) const
{
    for (const auto& mod : mods())
    {
        if (mod->key() == key)
        {
            return true;
        }
    }
    return false;
}

QList<QSharedPointer<Mod>> Repository::mods() const
{
    QList<QSharedPointer<Mod>> rVal;
    rVal.reserve(modAdapters_.size());
    for (const auto& item : modAdapters_)
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
    retVal.reserve(modAdapters_.size());
    for (const auto& item : modAdapters_)
    {
        retVal.append(item.get());
    }
    return retVal;
}
