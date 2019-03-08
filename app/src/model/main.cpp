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
    LOG << "Version: " << VERSION_CHARS;
    Global::workerThread = new QThread();
    Global::workerThread->setObjectName("workerThread");
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
    #ifndef QT_DEBUG
            AfiSyncLogger* logger = initStandalone();
    #endif

    MainWindow* mainWindow = new MainWindow();
    TreeModel* treeModel = generalInit(mainWindow);
    mainWindow->show();
    mainWindow->init(treeModel, new SettingsUiModel());
    mainWindow->treeWidget()->setRepositories(treeModel->repositories());
    QObject::connect(treeModel, &TreeModel::repositoriesChanged,
                     mainWindow->treeWidget(), &AsTreeWidget::setRepositories);

    const int rVal = app.exec();
    #ifndef QT_DEBUG
        delete logger;
    #endif
    // Needs to be done before shutting down workerThread
    treeModel->stopUpdates();
    delete mainWindow;
    Global::workerThread->quit();
    Global::workerThread->wait(1000);
    Global::workerThread->terminate();
    Global::workerThread->wait(1000);
    return rVal;
}

int cli(int argc, char* argv[])
{
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
                     });
    //Linux ctrl+c compatability
    CleanExit cleanExit;
    Q_UNUSED(cleanExit);

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
    const QString deltaPath = parser.value("delta-hash");
    if (!deltaPath.isEmpty())
    {
        LOG << "Hash for " << deltaPath << " is " << AHasher::hash(deltaPath);
        return 0;
    }

    //Delta patching
    const QSet<QString> missingArgs = DELTA_ARGS.toSet() - parser.optionNames().toSet();
    if (missingArgs.size() < DELTA_ARGS.size())
    {
        if (missingArgs.size() != 0)
        {
            return 2;
        }
        QString oldPath = parser.value(DELTA_ARGS.at(0));
        QString newPath = parser.value(DELTA_ARGS.at(1));
        QString patchesPath = parser.value(DELTA_ARGS.at(2));
        //TODO: This should be done with static functions
        DeltaPatcher dp(patchesPath, libtorrent::torrent_handle());
        dp.delta(oldPath, newPath);

        return 0;
    }
    LOG << "Fail " << missingArgs.size() << " " << missingArgs;
    TreeModel* model = generalInit();

    // Mirror
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
    return app.exec();
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        return cli(argc, argv);
    }
    return gui(argc, argv);
}
