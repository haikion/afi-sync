#include "settingsmodel.h"
#include "repository.h"
#include "installer.h"
#include "debug.h"

void Installer::install(const Mod* mod)
{
    QString modPath = SettingsModel::modDownloadPath() + "/" + mod->name();
    //TeamSpeak 3 plugins
    QDir tsDir(modPath + "/teamspeak 3 client");
    install(tsDir, SettingsModel::teamSpeak3Path());

    //User config
    QDir modUserConfig(modPath + "/userconfig");
    install(modUserConfig, QDir(SettingsModel::arma3Path() + "/userconfig"));

    //User config 2
    QDir modUserConfig2(modPath + "/store/userconfig");
    install(modUserConfig2, QDir(SettingsModel::arma3Path() + "/userconfig"));
}

void Installer::install(const QDir& src, const QDir& dst)
{
    if (!src.exists() || !dst.exists())
    {
        DBG << "Nothing to install src =" << src.absolutePath();
        return;
    }
    DBG << "Installing" << src.absolutePath()
             << "to" << dst.absolutePath();
    copyRecursively(src.absolutePath(), dst.absolutePath());
}

//https://qt.gitorious.org/qt-creator/qt-creator/source/1a37da73abb60ad06b7e33983ca51b266be5910e:src/app/main.cpp#L13-189
// taken from utils/fileutils.cpp. We can not use utils here since that depends app_version.h.
bool Installer::copyRecursively(const QString& srcFilePath, const QString& tgtFilePath)
{
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        targetDir.cdUp();
        QString dirName = QFileInfo(tgtFilePath).fileName();
        if (!targetDir.exists(dirName) && !targetDir.mkdir(dirName))
        {
            DBG << "Failure to create:" << dirName;
            return false;
        }
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString& fileName, fileNames) {
            const QString newSrcFilePath
                    = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                    = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        QFile tgtFile(tgtFilePath);
        DBG << "Installing: " << tgtFilePath;
        if (tgtFile.exists())
        {
            DBG << "Overwriting file: " << tgtFilePath;
            tgtFile.remove();
        }
        if (!QFile::copy(srcFilePath, tgtFilePath))
        {
            DBG << "Failure to copy " << srcFilePath << "to" << tgtFilePath;
            return false;
        }
    }
    return true;
}
