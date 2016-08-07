#include <csignal>
#include <QCoreApplication>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QtQml>
#include <QSettings>
#include <QDir>
#include <QTextStream>
#include <QCommandLineParser>
#include "apis/libtorrent/deltapatcher.h"
#include "global.h"
#include "treemodel.h"
#include "settingsmodel.h"
#include "debug.h"
#include "processmonitor.h"

static const QString LOG_FILE = "afisync.log";
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
    DBG;
    Q_UNUSED(scriptEngine)
    Global::model = new TreeModel(engine);

    return Global::model;
}

static QObject* getSettingsModel(QQmlEngine* engine, QJSEngine* scriptEngine)
{
    Q_UNUSED(scriptEngine)

    return new SettingsModel(engine);
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

    *Global::logStream << msg << "\n";
}

void createLogFile()
{
    QFile* file = new QFile(LOG_FILE);
    file->open(QIODevice::WriteOnly | QIODevice::Append);
    Global::logStream = new QTextStream(file);
}

int gui(int argc, char* argv[])
{
    #ifndef QT_DEBUG
        createLogFile();
        qInstallMessageHandler(messageHandler);
    #endif
    DBG << "\nAFISync" << Constants::VERSION_STRING << "started";
    qmlRegisterSingletonType<TreeModel>("org.AFISync", 0, 1, "TreeModel", getTreeModel);
    qmlRegisterSingletonType<SettingsModel>("org.AFISync", 0, 1, "SettingsModel", getSettingsModel);
    qmlRegisterSingletonType<ProcessMonitor>("org.AFISync", 0, 1, "ProcessMonitor", getProcessMonitor);
    DBG << "QML Singletons registered";
    QApplication app(argc, argv);
    DBG << "QGuiApplication created";
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/SplashScreen.qml")));
    DBG << "QML Engine loaded";

    int rVal = app.exec();
    DBG << "END";
    return rVal;
}

int cli(int argc, char* argv[])
{
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

    QCoreApplication app(argc, argv);

    QStringList args;
    for (int i = 0; i < argc; ++i)
    {
        DBG << argv[i];
        args.append(argv[i]);
    }
    parser.process(args);
    DBG << parser.errorText();

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
        DeltaPatcher dp(patchesPath);
        dp.delta(oldPath, newPath);

        return 0;
    }
    DBG << "FAIL" << missingArgs.size() << missingArgs;

    QString username, password, directory;
    unsigned port;
    directory = parser.value("mirror");
    port = parser.value("port").toInt();
    QFileInfo dir(directory);
    QString modDownloadPath = dir.absoluteFilePath();
    if (!dir.isDir() || !dir.isWritable())
    {
        qDebug() << "Invalid path:" << modDownloadPath;
        QCoreApplication::exit(2);
    }
    DBG << "Setting mod download path:" << modDownloadPath;
    SettingsModel::setModDownloadPath(modDownloadPath);
    SettingsModel::setPort(QString::number(port));

    Global::guiless = true;
    TreeModel* model = new TreeModel(port, &app);
    DBG << "model created";
    model->enableRepositories();
    return app.exec();
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("AFISync");
    QCoreApplication::setOrganizationDomain("armafinland.fi");
    QCoreApplication::setApplicationName("AFISync");
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, Constants::SETTINGS_PATH);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    if (argc > 1)
    {
        return cli(argc, argv);
    }
    return gui(argc, argv);
}
