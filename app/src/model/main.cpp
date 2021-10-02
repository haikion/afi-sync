#include <csignal>
#include <libtorrent/torrent_handle.hpp>
#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QMessageBox>
#include "../view/mainwindow.h"
#include "afisynclogger.h"
#include "apis/libtorrent/ahasher.h"
#include "apis/libtorrent/deltapatcher.h"
#include "apis/libtorrent/libtorrentapi.h"
#include "constantsmodel.h"
#include "crashhandler/crashhandler.h"
#include "fileutils.h"
#include "global.h"
#include "processmonitor.h"
#include "settingsmodel.h"
#include "settingsuimodel.h"
#include "treemodel.h"
#include "version.h"
#include "deletabledetector.h"

static const QStringList DELTA_ARGS = {"old-path", "new-path", "output-path"};

struct CleanExit
{
    CleanExit()
    {
        signal(SIGINT, &CleanExit::exitQt);
        signal(SIGTERM, &CleanExit::exitQt);
    }

    static void exitQt(int sig)
    {
        Q_UNUSED(sig);
        QCoreApplication::exit(0);
    }
};

AfiSyncLogger* initStandalone()
{
    #ifdef Q_OS_WIN
        Breakpad::CrashHandler::instance()->Init(QStringLiteral("."));
    #endif
    AfiSyncLogger* logger = new AfiSyncLogger();
    logger->initFileLogging();
    return logger;
}

TreeModel* generalInit(QObject* parent = nullptr)
{
    //Linux ctrl+c and kill compatability
    CleanExit cleanExit;
    Q_UNUSED(cleanExit);

    LOG << "Version: " << VERSION_CHARS;
    Global::workerThread = new QThread();
    Global::workerThread->setObjectName("AFISync worker thread");
    Global::workerThread->start();
    LOG << "Worker thread started";
    Global::model = new TreeModel(parent);
    SettingsModel::initBwLimits();
    return Global::model;
}

int gui(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (ProcessMonitor::afiSyncRunning())
    {
        QMessageBox msgBox;
        msgBox.setText("AFISync process is already running.");
        msgBox.exec();
        exit(1);
    }
    if (ProcessMonitor::arma3Running())
    {
        QMessageBox msgBox;
        msgBox.setText("AFISync cannot be started while Arma 3 is running.");
        msgBox.exec();
        exit(2);
    }
    AfiSyncLogger* logger = initStandalone();

    MainWindow* mainWindow = new MainWindow();
    TreeModel* treeModel = generalInit(mainWindow);
    mainWindow->show();
    mainWindow->init(treeModel, new SettingsUiModel());
    mainWindow->treeWidget()->setRepositories(treeModel->repositories());
    QObject::connect(treeModel, &TreeModel::repositoriesChanged,
                     mainWindow->treeWidget(), &AsTreeWidget::setRepositories);

    const int rVal = app.exec();
    // Needs to be done before shutting down workerThread
    treeModel->stopUpdates();
    delete mainWindow;
    delete logger;
    Global::workerThread->quit();
    Global::workerThread->wait(20000);
    Global::workerThread->terminate();
    // TODO: Uncomment or remove line below
    // Global::workerThread->wait(1000);
    return rVal;
}

int cli(int argc, char* argv[])
{
    static const QString MOD = "mod";
    static const QString PATCH = "patch";

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
                          {"mirror", "Mirror all files into <directory>. Current value: " + SettingsModel::modDownloadPath()
                           , "directory", SettingsModel::modDownloadPath()},
                          {"port", "External Port. Current value: " + SettingsModel::port()
                           , "port", SettingsModel::port()},
                          {"delta-hash", "Prints delta hash for the mod. Used for debugging", "mod directory", "."},
                          {DELTA_ARGS.at(0), "Delta patch generation:  Defines path to old version", DELTA_ARGS.at(0)},
                          {DELTA_ARGS.at(1), "Delta patch generation:  Defines path to new version", DELTA_ARGS.at(1)},
                          {DELTA_ARGS.at(2), "Delta patch generation:  Defines path to patches directory", DELTA_ARGS.at(2)},
                          {PATCH, "Delta Patching: Patches mod directory with given patch file. Usage: ./AFISync --patch patch.7z --mod @testmod", PATCH},
                          {MOD, "Delta Patching: Old mod directory", MOD}
                     });

    QStringList args;
    for (int i = 0; i < argc; ++i)
    {
        args.append(argv[i]);
    }
    parser.process(args);
    const QString errorText = parser.errorText();
    if (!errorText.isEmpty())
    {
        LOG << parser.errorText();
        return 1;
    }

    //Delta hash
    if (parser.isSet("delta-hash"))
    {
        const QString deltaPath = parser.value("delta-hash");
        LOG << "Hash for " << deltaPath << " is " << AHasher::hash(deltaPath);
        return 0;
    }
    if (parser.isSet(PATCH) || parser.isSet("mod")) {
        if (!parser.isSet(PATCH)) {
            LOG << "Missing patch";
            return 1;
        }
        if (!parser.isSet(MOD)) {
            LOG << "Missing mod";
            return 1;
        }
        const QString patch = parser.value("patch");
        const QString mod = parser.value("mod");
        DeltaPatcher dp;
        return dp.patchAbsolutePath(patch, mod) ? 0 : 1;
    }

    //Delta patching
    const QSet<QString> missingArgs = DELTA_ARGS.toSet() - parser.optionNames().toSet();
    if (missingArgs.size() < DELTA_ARGS.size())
    {
        if (missingArgs.size() != 0)
        {
            parser.showHelp(1);
        }
        const QString& oldPath = parser.value(DELTA_ARGS.at(0));
        const QString& newPath = parser.value(DELTA_ARGS.at(1));
        const QString& patchesPath = parser.value(DELTA_ARGS.at(2));
        DeltaPatcher dp(patchesPath, libtorrent::torrent_handle());
        dp.delta(oldPath, newPath);

        return 0;
    }
    LOG << "Fail " << missingArgs.size() << " " << missingArgs;
    // Mirror
    if (parser.isSet("mirror"))
    {
        TreeModel* model = generalInit();
        QFileInfo dir(parser.value("mirror"));
        QString modDownloadPath = dir.absoluteFilePath();
        if (!dir.isDir() || !dir.isWritable())
        {
            LOG << "Invalid path:" << modDownloadPath;
            QCoreApplication::exit(2);
        }
        LOG << "Setting mod download path: " << modDownloadPath;
        SettingsModel::setModDownloadPath(modDownloadPath);

        Global::guiless = true;
        SettingsModel::setDeltaPatchingEnabled(true);
        LOG << "Delta updates enabled due to the mirror mode.";
        SettingsModel::setPort(parser.value("port"), true);
        model->enableRepositories();
        return QCoreApplication::exec();
    }
    parser.showHelp(3);
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        return cli(argc, argv);
    }
    return gui(argc, argv);
}
