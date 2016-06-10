#include <csignal>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QtQml>
#include <QSettings>
#include <QDir>
#include <QTextStream>
#include <QCommandLineParser>
#include "global.h"
#include "treemodel.h"
#include "settingsmodel.h"
#include "debug.h"
#include "processmonitor.h"

static const QString LOG_FILE = "afisync.log";

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

int gui(int argc, char* argv[])
{
    qmlRegisterSingletonType<TreeModel>("org.AFISync", 0, 1, "TreeModel", getTreeModel);
    qmlRegisterSingletonType<SettingsModel>("org.AFISync", 0, 1, "SettingsModel", getSettingsModel);
    qmlRegisterSingletonType<ProcessMonitor>("org.AFISync", 0, 1, "ProcessMonitor", getProcessMonitor);
    DBG << "QML Singletons registered";
    QGuiApplication app(argc, argv);
    DBG << "QGuiApplication created";
    QQmlApplicationEngine engine;
    //DBG << 3.2;
    engine.load(QUrl(QStringLiteral("qrc:/SplashScreen.qml")));
    DBG << "QML Engine loaded";

    return app.exec();
}

int cli(int argc, char* argv[], QCommandLineParser& parser)
{
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
    QString username, password, directory;
    unsigned port;
    //username = parser.value("username");
    //password = parser.value("password");
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
    TreeModel* model = new TreeModel("username", "password", port, &app);
    DBG << "model created";
    model->enableRepositories();
    return app.exec();
}

void createLogFile()
{
    QFile* file = new QFile(LOG_FILE);
    file->open(QIODevice::WriteOnly | QIODevice::Append);
    Global::logStream = new QTextStream(file);
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("AFISync");
    QCoreApplication::setOrganizationDomain("armafinland.fi");
    QCoreApplication::setApplicationName("AFISync");
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, Constants::SETTINGS_PATH);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
                          {"mirror", "Mirror all files into <directory>. Current value: " + SettingsModel::modDownloadPath()
                           , "directory", SettingsModel::modDownloadPath()},
                          {"port", "External Port. Current value: " + SettingsModel::port()
                           , "port", SettingsModel::port()},
                          //{"username", "Web interface username (deprecated)", "username", Constants::DEFAULT_USERNAME},
                          //{"password", "Web interface password (deprecated)", "password", Constants::DEFAULT_PASSWORD}
                      });
    #ifndef QT_DEBUG
        createLogFile();
        qInstallMessageHandler(messageHandler);
    #endif
    DBG << "\nAFISync" << Constants::VERSION_STRING << "started";
    if (argc > 1)
    {
        return cli(argc, argv, parser);
    }
    return gui(argc, argv);
}
