 #ifndef DELTAPATCHER_H
#define DELTAPATCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QThread>
#include <QDir>

class DeltaPatcher: public QObject
{
    Q_OBJECT

public:
    DeltaPatcher(const QString& patchesPath);
    ~DeltaPatcher();

    //Patches dir to latest version.
    void patch(const QString& modPath);
    //Creates a patch file synchronously.
    //Patch file is a 7zip archive consisting of all xdelta patches.
    bool delta(const QString& oldDir, QString laterDir);
    static int latestVersion(const QString& modName, const QStringList& fileNames);
    static QStringList filterPatches(const QString& modPath, const QStringList& allPatches);
    static bool copyRecursively(const QString& srcFilePath, const QString& tgtFilePath);

signals:
    void patched(bool success);

private:
    static const QString XDELTA_EXECUTABLE;
    static const QString SZIP_EXECUTABLE;
    static const int TIMEOUT;
    static const int MAX_MERGES;
    static const QString DELTA_POSTFIX;
    static const QString DELTA_EXTENSION;
    static const QString SEPARATOR;
    static const QString PATCH_DIR;

    QProcess* process_;
    QFileInfo* patchesFi_;
    QThread* thread_;

    //void merge(const QString& oldPatchZip, const QString& newPatchPath,
    //           const QString& newModDir) const;
    bool extract(const QString& zipPath);
    bool createDir(const QDir& dir) const;
    bool waitFinished(QProcess* process) const;
    bool patchExtracted(const QString& extractedPath, const QString& targetPath);
    void cleanUp(QDir& deltaDir, QDir& tmpDir);
    int latestVersion(const QString& modName) const;
    //Applies single patch to mod dir
    bool patch(const QString& patch, const QString& modPath);

private slots:
    void threadConstructor(const QString& patchesPath);
    void patchDirSync(const QString& modPath);
    void compress(const QString& dir, const QString& archivePath);
    bool runCmd(const QString& cmd);

};

#endif // DELTAPATCHER_H
