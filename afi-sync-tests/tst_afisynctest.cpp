#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <libtorrent/torrent_info.hpp>

#include "../src/model/fileutils.h"
#include "../src/model/jsonreader.h"
#include "../src/model/treemodel.h"
#include "../src/model/repository.h"
#include "../src/model/global.h"
#include "../src/model/settingsmodel.h"
#include "../src/model/apis/libtorrent/libtorrentapi.h"
#include "../src/model/apis/libtorrent/ahasher.h"
#include "../src/model/apis/libtorrent/deltadownloader.h"
#include "../src/model/apis/libtorrent/deltapatcher.h"
#include "../src/model/apis/libtorrent/deltamanager.h"
#include "../src/model/console.h"
#include "../src/model/jsonreader.h"
#include "../src/model/fileutils.h"
#include "../src/model/afisynclogger.h"
#include "../src/model/modadapter.h"
#include "../src/model/deletabledetector.h"

//File paths etc..
static const QString PATCHING_FILES_PATH = "patching";
static const QString TMP_PATH = "temp";
static const QString TORRENT_2 = PATCHING_FILES_PATH  + "/afisync_patches_1.torrent";
static const QString VT5_TORRENT_PATH = "torrents/@vt5_1.torrent";
static const QString MOD1_3_NAME = "@mod1_3.torrent";
static const QString TORRENT_PATH_MOD1_3 = PATCHING_FILES_PATH + "/" + MOD1_3_NAME;
static const QString TORRENT_PATH_MOD1_3_TMP = TMP_PATH + "/" + MOD1_3_NAME;
static const QString MOD_NAME_1 = "@mod1";
static const QString MOD_PATH_1 = TMP_PATH + "/1/" + MOD_NAME_1;
static const QString MOD_PATH_2 = TMP_PATH + "/2/" + MOD_NAME_1;
static const QString MOD_PATH_3 = TMP_PATH + "/3/" + MOD_NAME_1;
static const QString MOD_PATH_4 = TMP_PATH + "/sameHash3/" + MOD_NAME_1;
static const QString DELTA_PATCH_NAME = "afisync_patches";
static const QString PATCHES_PATH = TMP_PATH + "/" + DELTA_PATCH_NAME;
//file copy tests
static const QString SRC_PATH = PATCHING_FILES_PATH;
static const QString DEEP_PATH = TMP_PATH + "/deeper";


class AfiSyncTest : public QObject
{
    Q_OBJECT

public:
    AfiSyncTest();

private Q_SLOTS:
    //Helper functions
    void startTest();
    void cleanupTest();

    //Console
    void simpleCmd();
    void simpleCmdNeg();

    //FileUtils tests
    void copy();
    void rmCi();
    void rmCiDifficult();
    void copyDeep();
    void noSuchDir();
    void copyDeepOverwrite();

    //Mod tests
    void modFilesRemoved();

    //Repo tests
    void repoTickedFalseDefault();
    void repoCheckboxClicked();

    //Size tests
    void sizeBasic();
    void repoSize();
    void noSize();
    void sizeStringB();
    void sizeStringMB();
    void sizeStringGB();
    void sizeStringGB2();
    void sizeString0();
    void sizeOverflow();

    //Delta patch tests
    void beforeDelta();
    void afterDelta();
    void delta();
    void deltaIdentical();
    void deltaHashCollision();
    void chainDelta();
    void patch();
    void torrentName();
    void patchAvailable();
    void patchAvailableNegativeName();
    void patchAvailableNegativeVer();
    void managerPatchAvailable();
    void managerPatchAvailableNeg();
    void managerContainsNeg();

    //DeletableDetector tests
    void deletables();

private:
    ISync* sync_;
    TreeModel* model_;
    JsonReader reader_;
    //Delta patching
    libtorrent::torrent_handle handle_;
    libtorrent::session* session_;
    SettingsModel* settings_;
    QThread* workerThreadCache;

    libtorrent::torrent_handle createHandle(const QString& url = QString(), const QString& modDownloadPath = TMP_PATH);
};


AfiSyncTest::AfiSyncTest():
   sync_(nullptr),
   model_(nullptr),
   session_(nullptr),
   settings_(new SettingsModel())
{
   QDir().mkdir(TMP_PATH + "/1");
   settings_->setModDownloadPath(TMP_PATH + "/1");

   Global::workerThread = new QThread();
   Global::workerThread->setObjectName("workerThread");
   Global::workerThread->start();
   // Used to skip thread asserts
   workerThreadCache = Global::workerThread;
   LOG << "Worker thread started";
   Global::sync = new LibTorrentApi();
   SettingsModel::initBwLimits();
}

//Helper functions

void AfiSyncTest::startTest()
{
    if (!sync_)
    {
        QDir(SettingsModel::syncSettingsPath()).removeRecursively();
        sync_ = new LibTorrentApi();
    }
    qDebug() << "END OF INIT TEST CASE";
}

void AfiSyncTest::cleanupTest()
{
    QCoreApplication::processEvents();
    if (model_)
    {
        delete model_;
        model_ = nullptr;
    }
    FileUtils::safeRemove("settings/AFISync/AFISync.ini");
}

libtorrent::torrent_handle AfiSyncTest::createHandle(const QString& url, const QString& modDownloadPath)
{
    using namespace libtorrent;
    namespace lt = libtorrent;

    settings_pack sett;
    sett.set_str(settings_pack::listen_interfaces, "0.0.0.0:6881");
    session_ = new lt::session(sett);
    error_code ec;
    if (ec)
    {
        fprintf(stderr, "failed to open listen socket: %s\n", ec.message().c_str());
        return libtorrent::torrent_handle();
    }
    add_torrent_params p;
    p.save_path = modDownloadPath.toStdString();
    if (url.size() != 0)
    {
        p.url = url.toStdString();
    }
    else
    {
        p.ti = boost::make_shared<torrent_info>(TORRENT_2.toStdString(), boost::ref(ec), 0);
    }
    if (ec)
    {
        fprintf(stderr, "%s\n", ec.message().c_str());
        return libtorrent::torrent_handle();
    }
    lt::torrent_handle handle = session_->add_torrent(p, ec);
    if (ec)
    {
        fprintf(stderr, "%s\n", ec.message().c_str());
        return handle;
    }
    //Wait for torrent file to download.
    lt::torrent_status status = handle.status();
    for (int a = 0; a < 100 && status.state == lt::torrent_status::downloading_metadata; ++a)
    {
        QThread::msleep(100);
        status = handle.status();
    }
    if (status.state == lt::torrent_status::downloading_metadata)
    {
        qDebug() << "ERROR: Failure downloading torrent from" << url << ":" << status.error.c_str();
        return lt::torrent_handle();
    }

    return handle;
}

//LibTorrent Delta Patching Tests

//Creates clean directory and file stucture for delta patch tests.
void AfiSyncTest::beforeDelta()
{
    cleanupTest();

    FileUtils::copy(PATCHING_FILES_PATH, TMP_PATH);
    FileUtils::copy(PATCHES_PATH, TMP_PATH + "/1/" + DELTA_PATCH_NAME);
    FileUtils::copy(PATCHES_PATH, TMP_PATH + "/2/" + DELTA_PATCH_NAME);
    FileUtils::copy(PATCHES_PATH, TMP_PATH + "/3/" + DELTA_PATCH_NAME);
}

void AfiSyncTest::afterDelta()
{
    QDir tmpDir = QDir(TMP_PATH);
    FileUtils::safeRemoveRecursively(tmpDir);
}

void AfiSyncTest::delta()
{
    beforeDelta();

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH, libtorrent::torrent_handle());
    QDir patchesDir(PATCHES_PATH);
    patchesDir.removeRecursively();
    patchesDir.mkpath(".");
    bool rVal = patcher->delta(MOD_PATH_1, MOD_PATH_2);
    delete patcher;
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::deltaIdentical()
{
    beforeDelta();

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH, libtorrent::torrent_handle());
    QDir modDir2(MOD_PATH_2);
    FileUtils::safeRemoveRecursively(modDir2);
    modDir2.mkpath(".");
    FileUtils::copy(MOD_PATH_1, MOD_PATH_1);
    bool rVal = patcher->delta(MOD_PATH_1, MOD_PATH_2);
    delete patcher;
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::deltaHashCollision()
{
    beforeDelta();

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH, libtorrent::torrent_handle());
    QDir modDir2(MOD_PATH_2);
    FileUtils::safeRemoveRecursively(modDir2);
    modDir2.mkpath(".");
    bool rVal = patcher->delta(MOD_PATH_3, MOD_PATH_4);
    delete patcher;
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::chainDelta()
{
    beforeDelta();

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH, libtorrent::torrent_handle());
    QDir patchesDir(PATCHES_PATH);
    patchesDir.removeRecursively();
    patchesDir.mkpath(".");
    bool rVal1 = patcher->delta(MOD_PATH_1, MOD_PATH_2);
    bool rVal2 = patcher->delta(MOD_PATH_2, MOD_PATH_3);
    delete patcher;
    afterDelta();
    QVERIFY(rVal1);
    QVERIFY(rVal2);
}

void AfiSyncTest::patch()
{
    beforeDelta();

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH, libtorrent::torrent_handle());
    QEventLoop loop;
    QObject::connect(patcher, &DeltaPatcher::patched, &loop, &QEventLoop::quit);
    patcher->patch(MOD_PATH_1);
    qDebug() << "Waiting... patched signal";
    loop.exec();
    qDebug() << "Patched signal received";
    QString hash1 = AHasher::hash(MOD_PATH_1);
    QString hash2 = AHasher::hash(MOD_PATH_3);
    delete patcher;
    afterDelta();
    QCOMPARE(hash1, hash2);
}

void AfiSyncTest::torrentName()
{
    beforeDelta();

    handle_ = createHandle();
    QVERIFY(handle_.torrent_file() != nullptr);
    QString name = QString::fromStdString(handle_.torrent_file()->name());
    afterDelta();
    QCOMPARE(name, DELTA_PATCH_NAME);
}

void AfiSyncTest::patchAvailable()
{
    beforeDelta();

    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaDownloader upd(handle_);
    bool rVal = upd.patchAvailable(MOD_NAME_1);
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::patchAvailableNegativeName()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaDownloader upd(handle_);
    bool rVal = upd.patchAvailable("@mod5");
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::patchAvailableNegativeVer()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/3");
    DeltaDownloader upd(handle_);
    bool rVal = upd.patchAvailable(MOD_NAME_1);
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::managerPatchAvailable()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    Global::workerThread = QThread::currentThread(); // Skip thread asserts
    DeltaManager* manager = new DeltaManager(handle_);
    Global::workerThread = workerThreadCache;
    bool rVal = manager->patchAvailable(MOD_NAME_1);
    delete manager;
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::managerPatchAvailableNeg()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    Global::workerThread = QThread::currentThread(); // Skip thread asserts
    DeltaManager* manager = new DeltaManager(handle_);
    Global::workerThread = workerThreadCache;
    bool rVal = manager->patchAvailable("@doesnotexist");
    delete manager;
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::managerContainsNeg()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    Global::workerThread = QThread::currentThread(); // Skip thread asserts
    DeltaManager* manager = new DeltaManager(handle_);
    Global::workerThread = workerThreadCache;
    bool rVal = manager->contains("http://doesnotexist.org/no.torrent");
    delete manager;
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::deletables()
{
    Mod* commonMod = new Mod("name1", "key2", Global::sync);
    Mod* mod1 = new Mod(MOD_NAME_1, "key2", Global::sync);
    Mod* mod2 = new Mod("name3", "key3", Global::sync);
    Repository* repo1 = new Repository("repo1", "address", 1024, "password1", Global::sync);
    Repository* repo2 = new Repository("repo2", "address", 1024, "password1", Global::sync);
    new ModAdapter(commonMod, repo1, false, 0);
    new ModAdapter(commonMod, repo2, false, 0);
    new ModAdapter(mod1, repo1, false, 0);
    new ModAdapter(mod2, repo1, false, 0);
    QList<Repository*> repositories;
    repositories.append(repo1);
    repositories.append(repo2);

    DeletableDetector::printDeletables(repositories);
    delete mod1;
    delete mod2;
}

//FileUtils tests

void AfiSyncTest::copy()
{
    FileUtils::copy(SRC_PATH, TMP_PATH);
    QDir dstDir(TMP_PATH);
    bool rVal = dstDir.exists();
    bool childDirExists = QFileInfo(TMP_PATH + "/1").isDir();
    FileUtils::safeRemoveRecursively(dstDir); //Cleanup
    QVERIFY(rVal);
    QVERIFY(childDirExists);
}

void AfiSyncTest::rmCi()
{
    FileUtils::copy(SRC_PATH, TMP_PATH);
    FileUtils::rmCi(TORRENT_PATH_MOD1_3_TMP);
    bool fileRemoved = !QFileInfo(TORRENT_PATH_MOD1_3_TMP).exists();
    FileUtils::safeRemoveRecursively(TMP_PATH); //Cleanup
    QVERIFY(fileRemoved);
}

void AfiSyncTest::rmCiDifficult()
{
    static const QString PATH = "späced päth";
    static const QString FILE_PATH = PATH + "/" + MOD1_3_NAME;

    QDir().mkpath(PATH);
    FileUtils::copy(SRC_PATH, PATH);
    FileUtils::rmCi(FILE_PATH);
    bool fileRemoved = !QFileInfo(FILE_PATH).exists();
    FileUtils::safeRemoveRecursively(PATH);
    QVERIFY(fileRemoved);
}

void AfiSyncTest::copyDeep()
{
    FileUtils::copy(SRC_PATH, DEEP_PATH);
    QDir dstDir(DEEP_PATH);
    bool rVal = dstDir.exists();
    dstDir.removeRecursively(); //cleanup
    QVERIFY(rVal);
}

//Tests if dir is copied over dir when going to over 0 level.
void AfiSyncTest::copyDeepOverwrite()
{
    FileUtils::copy(SRC_PATH, TMP_PATH);
    FileUtils::copy(SRC_PATH, TMP_PATH);
    QDir dstDir(TMP_PATH);
    bool rVal = dstDir.exists();
    dstDir.removeRecursively(); //cleanup
    QVERIFY(rVal);
}

void AfiSyncTest::noSuchDir()
{
    QDir(TMP_PATH).removeRecursively();
    QString noExist = "noExist";
    FileUtils::copy("noExist", TMP_PATH);
    QVERIFY(!QFileInfo(TMP_PATH).exists());
}

void AfiSyncTest::simpleCmd()
{
    Console* cmd = new Console();
    QVERIFY(!cmd->runCmd("cd .."));
    cmd->deleteLater();
}

void AfiSyncTest::simpleCmdNeg()
{
    Console* cmd = new Console();
    QVERIFY(!cmd->runCmd("cd \"no such dir\""));
    cmd->deleteLater();
}

//Mod tests

//Tests if files get removed from mod directory
//if that was the only change done.

void AfiSyncTest::modFilesRemoved()
{
    startTest();
    FileUtils::copy("mods", TMP_PATH);
    settings_->setModDownloadPath(TMP_PATH + "/1extraFile");
    Repository* repo = new Repository("dummy2", "address", 1234, "", Global::sync);
    Mod* mod = new Mod("@mod1", TORRENT_PATH_MOD1_3, Global::sync);
    ModAdapter* adp = new ModAdapter(mod, repo, false, 0);
    Q_UNUSED(adp);
    //repo->checkboxClicked();
    QSet<QString> readys;
    readys.insert(SyncStatus::READY);
    readys.insert(SyncStatus::READY_PAUSED);
    while (!readys.contains(mod->statusStr()))
    {
        qDebug() << "Wating... mod status =" << mod->statusStr();
        QThread::sleep(1); //Wait for check to finish
    }
    const bool extraFileExists = QFileInfo(TMP_PATH + "/1extraFile/@mod1/extraFile.txt").exists();
    FileUtils::safeRemoveRecursively(TMP_PATH);
    QVERIFY(!extraFileExists);
    cleanupTest();
}

//Repository tests

void AfiSyncTest::repoTickedFalseDefault()
{
    cleanupTest();
    startTest();

    Repository* repo = new Repository("dummy3", "address", 1234, "", Global::sync);
    QVERIFY(!repo->ticked());

    cleanupTest();
}

void AfiSyncTest::repoCheckboxClicked()
{
    startTest();

    Repository* repo = new Repository("dummy4", "address", 1234, "", Global::sync);
    repo->checkboxClicked();
    QVERIFY(repo->ticked());

    cleanupTest();
}

//Size tests

void AfiSyncTest::sizeBasic()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod->setFileSize(quint64(1000));
    QVERIFY(mod->fileSize() == quint64(1000));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::repoSize()
{
    QSKIP("Server down");
    startTest();

    Repository* repo = new Repository("name", "address", 1234, "password", Global::sync);
    Mod* mod1 = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod1->setFileSize(quint64(1000));
    Mod* mod2 = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod2->setFileSize(quint64(3000));
    new ModAdapter(mod1, repo, false, 0);
    new ModAdapter(mod2, repo, false, 1);

    QVERIFY(repo->fileSize() == quint64(4000));

    cleanupTest();
}

void AfiSyncTest::noSize()
{
    QSKIP("Server down");
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    QVERIFY(mod->sizeStr() == 0);
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringB()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod->setFileSize(quint64(1000));
    QCOMPARE(mod->sizeStr(), QString("1000.00 B"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringMB()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod->setFileSize(quint64(21309));
    QCOMPARE(mod->sizeStr(), QString("20.81 kB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringGB()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod->setFileSize(quint64(3376414));
    QCOMPARE(mod->sizeStr(), QString("3.22 MB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringGB2()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod->setFileSize(quint64(33764140));
    QCOMPARE(mod->sizeStr(), QString("32.20 MB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeString0()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    QCOMPARE(mod->sizeStr(), QString("??.?? MB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeOverflow()
{
    startTest();

    Mod* mod = new Mod("@vt5", VT5_TORRENT_PATH, Global::sync);
    mod->setFileSize(quint64(13770848165));
    QCOMPARE(mod->sizeStr(), QString("12.83 GB"));
    mod->deleteLater();

    cleanupTest();
}

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
