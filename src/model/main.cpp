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

static const QString LOG_FILE = "afisync.log";

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
    DBG << "QML Singletons registered";
    QGuiApplication app(argc, argv);
    DBG << "QGuiApplication created";
    QQmlApplicationEngine engine;
    //DBG << 3.2;
    engine.load(QUrl(QStringLiteral("qrc:/SplashScreen.qml")));
    DBG << "QML Engine loaded";

    return app.exec();
}

int cli(int argc, char* argv[], const QCommandLineParser& parser)
{
    QString username, password, directory;
    unsigned port;
    username = parser.value("username");
    password = parser.value("password");
    directory = parser.value("directory");
    port = parser.value("port").toInt();
    QFileInfo dir(directory);
    if (!dir.isDir() || !dir.isWritable())
    {
        qDebug() << "Invalid path: " << dir.absolutePath();
        QCoreApplication::exit(2);
    }
    SettingsModel::setModDownloadPath(dir.absoluteFilePath());
    QCoreApplication app(argc, argv);
    Global::guiless = true;
    TreeModel* model = new TreeModel(username, password, port, &app);
    DBG << "model created";
    model->enableRepositories();
    Q_UNUSED(model)
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
    Global::runningTime->start();
    QCoreApplication::setOrganizationName("AFISync");
    QCoreApplication::setOrganizationDomain("armafinland.fi");
    QCoreApplication::setApplicationName("AFISync");
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, Constants::SETTINGS_PATH);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOptions({
                          {{"mirror", "directory"}, "Mirror all files into <directory>", "directory", SettingsModel::modDownloadPath()},
                          {"port", "Web interface port", "port", QString::number(Constants::DEFAULT_PORT)},
                          {"username", "Web interface username", "username", Constants::DEFAULT_USERNAME},
                          {"password", "Web interface password", "password", Constants::DEFAULT_PASSWORD}
                      });
    #ifndef QT_DEBUG
        createLogFile();
        qInstallMessageHandler(messageHandler);
    #endif
    DBG << "\nAFISync" << Constants::VERSION_STRING << "started";
    QStringList args;
    for (int i = 0; i < argc; ++i)
    {
        args.append(argv[i]);
    }
    parser.process(args);
    if (parser.isSet("mirror"))
    {
        return cli(argc, argv, parser);
    }
    return gui(argc, argv);
}
