#include <csignal>
#include <libtorrent/torrent_handle.hpp>
#include <QApplication>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QSettings>
#include <QTextStream>
#include <QtQml>
#include "afisynclogger.h"
#include "apis/libtorrent/deltapatcher.h"
#include "constantsmodel.h"
#include "crashhandler/crashhandler.h"
#include "fileutils.h"
#include "global.h"
#include "processmonitor.h"
#include "settingsmodel.h"
#include "treemodel.h"

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

static QObject* getTreeModel(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    LOG;
    Q_UNUSED(scriptEngine)
    Global::model = new TreeModel(engine);

    //This variable is used to locate afisync_header.png
    engine->rootContext()->setContextProperty("applicationDirPath", QGuiApplication::applicationDirPath());

    return Global::model;
}

static QObject* getSettingsModel(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(scriptEngine)

    //QQmlEngine tries to destroy singletons on destruction so real C++ singletons cannot be used.
    return new SettingsModel(engine);
}

static QObject* getConstantsModel(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(scriptEngine)

    //QQmlEngine tries to destroy singletons on destruction so real C++ singletons cannot be used.
    return new ConstantsModel(engine);
}

static QObject* getProcessMonitor(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(scriptEngine)

    return new ProcessMonitor(engine);
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)

    static QMutex mutex;
    QMutexLocker locker(&mutex);

    LOG << msg.toStdString() << "\n";
}

void initStandalone()
{
    #ifdef Q_OS_WIN
        Breakpad::CrashHandler::instance()->Init(QStringLiteral("."));
    #endif
    AfiSyncLogger::initFileLogging();
    qInstallMessageHandler(messageHandler);
}

int gui(int argc, char* argv[])
{
    QApplication app(argc, argv);

    #ifndef QT_DEBUG
        initStandalone();
    #endif
    LOG << "\nAFISync" << Constants::VERSION_STRING << "started";
    qmlRegisterSingletonType<TreeModel>("org.AFISync", 0, 1, "TreeModel", getTreeModel);
    qmlRegisterSingletonType<SettingsModel>("org.AFISync", 0, 1, "SettingsModel", getSettingsModel);
    qmlRegisterSingletonType<ProcessMonitor>("org.AFISync", 0, 1, "ProcessMonitor", getProcessMonitor);
    qmlRegisterSingletonType<ConstantsModel>("org.AFISync", 0, 1, "ConstantsModel", getConstantsModel);
    LOG << "QML Singletons registered";
    LOG << "QGuiApplication created";
    QQmlApplicationEngine engine;
    #ifdef STATIC_BUILD
        engine.setImportPathList(QStringList(QStringLiteral("qrc:/qml")));
    #endif
    engine.load(QUrl(QStringLiteral("qrc:/SplashScreen.qml")));
    LOG << "QML Engine loaded";

    const int rVal = app.exec();
    LOG << "END";
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
        LOG << argv[i];
        args.append(argv[i]);
    }
    parser.process(args);
    LOG << parser.errorText();

    //Delta patching
    QSet<QString> missingArgs = DELTA_ARGS.toSet() - parser.optionNames().toSet();
    if (missingArgs.size() < DELTA_ARGS.size())
    {
        if (missingArgs.size() != 0)
        {
            qDebug() << "ERROR: Missing arguments:" << missingArgs;
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
    LOG << "FAIL" << missingArgs.size() << missingArgs;

    QFileInfo dir(parser.value("mirror"));
    QString modDownloadPath = dir.absoluteFilePath();
    if (!dir.isDir() || !dir.isWritable())
    {
        qDebug() << "Invalid path:" << modDownloadPath;
        QCoreApplication::exit(2);
    }
    LOG << "Setting mod download path:" << modDownloadPath;
    SettingsModel::setModDownloadPath(modDownloadPath);

    Global::guiless = true;
    SettingsModel::setDeltaPatchingEnabled(true);
    LOG << "Delta updates enabled due to the mirror mode.";
    TreeModel* model = new TreeModel(&app, true);
    SettingsModel::setPort(parser.value("port"), true);
    LOG << "model created";
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
