/*
 * Performs delta patching of files. Works on file system level.
 */

#ifndef DELTAPATCHER_H
#define DELTAPATCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QDir>
#include <QQueue>
#include <QMutex>

#include "../../console.h"

class DeltaPatcher: public QObject
{
    Q_OBJECT

public:
    DeltaPatcher(const QString& patchesPath);
    DeltaPatcher();
    ~DeltaPatcher() override;

    //Patches dir to latest version.
    void patch(const QString& modPath);
    //Creates a patch file synchronously.
    //Patch file is a 7zip archive consisting of all xdelta patches.
    bool delta(const QString& oldModPath, QString laterDir);
    static QStringList filterPatches(const QString& modPath, const QStringList& allPatches);
    qint64 bytesPatched(const QString& modName);
    //Returns total number of bytes in a patch when patch is being applied.
    //Returns zero otherwise.
    qint64 totalBytes(const QString& modName);
    bool isPatching();
    void stop();    
    bool patching(const QString& modName);
    bool patchExtracting();
    bool patchAbsolutePath(const QString& patch, const QString& modPath);

signals:
    void patched(QString modPath, bool success);

private:
    static const QString XDELTA_EXECUTABLE;
    static const QString SZIP_EXECUTABLE;
    static const int TIMEOUT;
    static const QString DELTA_POSTFIX;
    static const QString DELTA_EXTENSION;
    static const QString SEPARATOR;
    static const QString PATCH_DIR;

    QAtomicInteger<qint64> bytesPatched_;
    std::atomic<bool> extractingPatches_;
    QAtomicInteger<qint64> totalBytes_;
    QFileInfo* patchesFi_;
    //Contains the name of the mod being patched.
    QString patchingMod_;
    Console* console_{nullptr};
    QMutex mutex_;

    bool extract(const QString& zipPath);
    [[nodiscard]] static bool createEmptyDir(QDir dir);
    bool patchExtracted(const QString& extractedPath, const QString& targetPath);
    static void cleanUp(QDir& deltaDir, QDir& tmpDir);
    [[nodiscard]] int latestVersion(const QString& modName) const;
    static int latestVersion(const QString& modName, const QStringList& fileNames);
    //Applies single patch to mod dir
    bool patch(const QString& patch, const QString& modPath);
    [[nodiscard]] static QStringList removePatchesFromLatest(const QString& latestPath, const QString& deltaPath);
    void applyPatches(const QString& modPath, QStringList patches);

private slots:
    void patchDirSync(const QString& modPath);
    void compress(const QString& dir, const QString& archivePath);
};

#endif // DELTAPATCHER_H
