/*
 * Performs delta patching of files. Works on file system level.
 */

#ifndef DELTAPATCHER_H
#define DELTAPATCHER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QThread>
#include <QDir>
#include <QQueue>
#include <QMutex>

#pragma warning(push, 0)
#include "libtorrent/torrent_handle.hpp"
#pragma warning(pop)

#include "../../console.h"

class DeltaPatcher: public QObject
{
    Q_OBJECT

public:
    DeltaPatcher(const QString& patchesPath, libtorrent::torrent_handle handle);
    ~DeltaPatcher();

    //Patches dir to latest version.
    void patch(const QString& modPath);
    //Creates a patch file synchronously.
    //Patch file is a 7zip archive consisting of all xdelta patches.
    bool delta(const QString& oldPath, QString laterDir);
    static QStringList filterPatches(const QString& modPath, const QStringList& allPatches);
    qint64 bytesPatched(const QString& modName);
    //Returns total number of bytes in a patch when patch is being applied.
    //Returns zero otherwise.
    qint64 totalBytes(const QString& modName);
    bool notPatching();
    void stop();    
    bool patching(const QString& modName);

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
    QAtomicInteger<qint64> totalBytes_;
    QFileInfo* patchesFi_;
    QThread thread_;
    //Contains the name of the mod being patched.
    QString patchingMod_;
    Console* console_;
    libtorrent::torrent_handle handle_;
    QMutex mutex_;

    bool extract(const QString& zipPath);
    bool createEmptyDir(QDir dir) const;
    bool patchExtracted(const QString& extractedPath, const QString& targetPath);
    void cleanUp(QDir& deltaDir, QDir& tmpDir);
    int latestVersion(const QString& modName) const;
    static int latestVersion(const QString& modName, const QStringList& fileNames);
    //Applies single patch to mod dir
    bool patch(const QString& patch, const QString& modPath);
    QStringList removePatchesFromLatest(const QString& latestPath, const QString& deltaPath) const;
    void applyPatches(const QString& modPath, QStringList patches, int attempts);

private slots:
    void threadConstructor(const QString& patchesPath);
    void patchDirSync(const QString& modPath);
    void compress(const QString& dir, const QString& archivePath);
};

#endif // DELTAPATCHER_H
