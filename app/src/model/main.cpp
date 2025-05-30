#include <csignal>

#include <libtorrent/torrent_handle.hpp>

#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QStringLiteral>

#include "../view/mainwindow.h"
#include "afisynclogger.h"
#include "apis/libtorrent/ahasher.h"
#include "apis/libtorrent/deltapatcher.h"
#include "fileutils.h"
#include "global.h"
#include "processmonitor.h"
#include "settingsmodel.h"
#include "treemodel.h"
#include "version.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstdio>
#endif

using namespace Qt::StringLiterals;

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
    if (SettingsModel::instance().versionCheckEnabled())
    {
        const auto versionCheck = Global::model->checkVersion();
        if (versionCheck.updateAvailable()) {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setWindowTitle(u"Version Update Available"_s);
            msgBox.setText(u"A new version of AFISync is available."_s);

            // Display the current version and the latest version
            auto message = u"Current version: %1.%2\nLatest version: %3\n\n"
                           "Would you like to visit the download page for the latest version?\n\n"_s
                               .arg(MAJOR_VERSION).arg(MINOR_VERSION)
                               .arg(versionCheck.latestVersion());
            msgBox.setInformativeText(message);
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);

            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                QUrl url{versionCheck.versionUrl()};
                if (url.scheme() == "http"_L1 || url.scheme() == "https"_L1) {
                    QDesktopServices::openUrl(url);
                } else {
                    LOG_WARNING << "Incorrect url given: " << versionCheck.versionUrl();
                }
            }
        }
    }

    SettingsModel::instance().initBwLimits();
    return Global::model;
}

int gui(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (ProcessMonitor::afiSyncRunning())
    {
        QMessageBox msgBox;
        msgBox.setText(u"AFISync process is already running."_s);
        msgBox.exec();
        exit(1);
    }
    if (ProcessMonitor::arma3Running())
    {
        QMessageBox msgBox;
        msgBox.setText(u"AFISync cannot be started while Arma 3 is running."_s);
        msgBox.exec();
        exit(2);
    }
    AfiSyncLogger logger;
#ifndef QT_DEBUG
    logger.initFileLogging();
#else
    logger.initConsoleLogging();
#endif

    auto mainWindow = new MainWindow();
    TreeModel* treeModel = generalInit(mainWindow);
    mainWindow->show();
    mainWindow->init(treeModel);
    mainWindow->treeWidget()->setRepositories(treeModel->repositories());
    QObject::connect(treeModel, &TreeModel::repositoriesChanged,
                     mainWindow->treeWidget(), &AsTreeWidget::setRepositories);

    const int rVal = app.exec();
    // Needs to be done before shutting down workerThread
    treeModel->stopUpdates();
    delete mainWindow;
    Global::workerThread->quit();
    Global::workerThread->wait(20000);
    Global::workerThread->terminate();
    LOG << "Safe subpaths: " << FileUtils::getSafeSubPaths();
    return rVal;
}

int cli(int argc, char* argv[])
{
    static const QString MOD = u"mod"_s;
    static const QString PATCH = u"patch"_s;

// On Windows, if CLI mode is requested, ensure a console is attached.
#ifdef Q_OS_WIN
    if (argc > 1) {
        // First, try to attach to the parent console. If that fails, allocate a new one.
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
            AllocConsole();
        }
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    QCoreApplication app(argc, argv);
    static SettingsModel& settings = SettingsModel::instance();
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
                          {"mirror", "Mirror all files into <directory>. Current value: " + settings.modDownloadPath()
                           , "directory", settings.modDownloadPath()},
                          {"port", "External Port. Current value: " + settings.port()
                           , "port", settings.port()},
                          {"delta-hash", "Prints delta hash for the mod. Used for debugging", "mod directory", "."},
                          {"delta-hash-legacy", "Prints delta hash for the mod. Used for debugging", "mod directory", "."},
                          {DELTA_ARGS.at(0), "Delta patch generation:  Defines path to old version", DELTA_ARGS.at(0)},
                          {DELTA_ARGS.at(1), "Delta patch generation:  Defines path to new version", DELTA_ARGS.at(1)},
                          {DELTA_ARGS.at(2), "Delta patch generation:  Defines path to patches directory", DELTA_ARGS.at(2)},
                          {PATCH, "Delta Patching: Patches mod directory with given patch file. Usage: AFISync.exe --patch patch.7z --mod @testmod", PATCH},
                          {MOD, "Delta Patching: Old mod directory", MOD}
                     });

    QStringList args;
    args.reserve(argc);
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
    if (parser.isSet(u"delta-hash"_s)) {
        const QString deltaPath = parser.value(u"delta-hash"_s);
        LOG << "Hash for " << deltaPath << " is " << AHasher::hash(deltaPath);
        return 0;
    }
    if (parser.isSet(u"delta-hash-legacy"_s))
    {
        const QString deltaPath = parser.value(u"delta-hash"_s);
        LOG << "Hash for " << deltaPath << " is " << AHasher::legacyHash(deltaPath);
        return 0;
    }
    if (parser.isSet(PATCH) || parser.isSet(u"mod"_s)) {
        if (!parser.isSet(PATCH)) {
            LOG << "Missing patch";
            return 1;
        }
        if (!parser.isSet(MOD)) {
            LOG << "Missing mod";
            return 1;
        }
        const QString patch = parser.value(u"patch"_s);
        const QString mod = parser.value(u"mod"_s);
        DeltaPatcher dp;
        return dp.patchAbsolutePath(patch, mod) ? 0 : 1;
    }

    //Delta patching
    const auto deltaArgsSet = QSet<QString>(DELTA_ARGS.begin(), DELTA_ARGS.end());
    const auto parserOptions = parser.optionNames();
    const auto parserOptionsSet = QSet<QString>(parserOptions.begin(), parserOptions.end());
    const QSet<QString> missingArgs = deltaArgsSet - parserOptionsSet;
    if (missingArgs.size() < DELTA_ARGS.size())
    {
        if (!missingArgs.isEmpty())
        {
            parser.showHelp(1);
        }
        const QString& oldPath = parser.value(DELTA_ARGS.at(0));
        const QString& newPath = parser.value(DELTA_ARGS.at(1));
        QString patchesPath = parser.value(DELTA_ARGS.at(2));
        if (patchesPath.endsWith('/') || patchesPath.endsWith('\\')) {
            patchesPath.remove(patchesPath.size()-1, 1);
        }
        FileUtils::appendSafePath(patchesPath);
        DeltaPatcher dp{patchesPath};
        dp.delta(oldPath, newPath);

        return 0;
    }
    LOG << "Fail " << missingArgs.size() << " " << missingArgs;
    // Mirror
    if (parser.isSet(u"mirror"_s)) {
        Global::isMirror = true;
        QFileInfo dir(parser.value(u"mirror"_s));
        QString modDownloadPath = dir.absoluteFilePath();
        if (!dir.isDir() || !dir.isWritable())
        {
            LOG << "Invalid path:" << modDownloadPath;
            QCoreApplication::exit(2);
        }
        LOG << "Setting mod download path: " << modDownloadPath;
        settings.setModDownloadPath(modDownloadPath);
        
        AfiSyncLogger logger;
        logger.initConsoleLogging();

        TreeModel* model = generalInit();

        settings.setDeltaPatchingEnabled(true);
        LOG << "Delta updates enabled due to the mirror mode.";
        settings.setPort(parser.value(u"port"_s), true);
        model->enableRepositories();
        return QCoreApplication::exec();
    }
    parser.showHelp(3);
}

int main(int argc, char* argv[])
{
    return argc > 1 ? cli(argc, argv) : gui(argc, argv);
}
