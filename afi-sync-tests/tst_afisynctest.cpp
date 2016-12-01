/*
 * Unit tests for AFISync
 * Make sure workingDirectory is defined as working directory.
 */

#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <libtorrent/torrent_info.hpp>

#include "../src/model/fileutils.h"
#include "../src/model/jsonreader.h"
#include "../src/model/rootitem.h"
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
#include "../src/model/debug.h"

//Delta patching consts
static const QString PATCHING_FILES_PATH = "patching";
static const QString TMP_PATH = "temp";
static const QString TORRENT_1 = "http://88.193.244.18/torrents/@vt5_1.torrent";
static const QString TORRENT_2 = PATCHING_FILES_PATH  + "/afisync_patches_1.torrent";
static const QString TORRENT_4 = "http://88.193.244.18/torrents/afisync_patches_1.torrent";
static const QString MOD_NAME_1 = "@mod1";
static const QString MOD_PATH_1 = TMP_PATH + "/1/" + MOD_NAME_1;
static const QString MOD_PATH_2 = TMP_PATH + "/2/" + MOD_NAME_1;
static const QString MOD_PATH_3 = TMP_PATH + "/3/" + MOD_NAME_1;
static const QString DELTA_PATCH_NAME = "afisync_patches";
static const QString PATCHES_PATH = TMP_PATH + "/" + DELTA_PATCH_NAME;
//file copy tests
static const QString SRC_PATH = PATCHING_FILES_PATH;
static const QString DST_PATH = TMP_PATH;
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

    //FileUtils tests
    void copy();
    void copyDeep();
    void noSuchDir();
    void copyDeepOverwrite();

    //LibTorrent tests
    void saveAndLoad();
    void getEta();
    void setFolderPaused();
    void getFolderKeys();
    void getFilesUpper();
    void addRemoveFolder();

    //Delta patch tests
    void beforeDelta();
    void afterDelta();
    void hash();
    void delta();
    void chainDelta();
    void patch();
    void torrentName();
    void patchAvailable();
    void patchAvailableNegativeName();
    void patchAvailableNegativeVer();
    void managerPatchAvailable();
    void managerPatchAvailableNeg();
    void managerContains();
    void managerContainsNeg();
    void managerPatch();
    void managerPatchNeg();
    void deltaDownloadDownloader();

    //JsonReader Tests
    void jsonReaderBasic();
    void jsonReader2Repos();
    void jsonReaderModUpdate();
    void jsonReaderModRemove();
    void jsonReaderRepoRename();
    void jsonReaderAddRepo();
    void jsonReaderRemoveRepo();
    void jsonReaderMoveMod();

private:
    ISync* sync_;
    TreeModel* model_;
    JsonReader reader_;
    RootItem* root_;
    //Delta patching
    libtorrent::torrent_handle handle_;
    libtorrent::session* session_;
    SettingsModel* settings_;

    libtorrent::torrent_handle createHandle(const QString& url = QString(), const QString& modDownloadPath = TMP_PATH);
};


AfiSyncTest::AfiSyncTest():
   sync_(nullptr),
   model_(nullptr),
   root_(nullptr),
   session_(nullptr),
   settings_(new SettingsModel(this))
{
   handle_ = createHandle();
   settings_->setModDownloadPath(TMP_PATH + "/1");
}

//Helper functions

void AfiSyncTest::startTest()
{
    if (!model_)
    {
        QDir(Constants::SYNC_SETTINGS_PATH).removeRecursively();
        model_ = new TreeModel();
        root_ = model_->rootItem();
        sync_ = root_->sync();
    }
    qDebug() << "END OF INIT TEST CASE";
}

void AfiSyncTest::cleanupTest()
{
    QEventLoop loop;
    DBG << "Main thread event process status:" << loop.processEvents();
    if (model_)
    {
        delete model_;
        model_ = nullptr;
    }
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

//Creates model, adds folder, deletes model, creates model again. Added folder should still exist.
void AfiSyncTest::saveAndLoad()
{
    QSKIP("Delay");

    ISync* sync = new LibTorrentApi();

    QDir().mkpath(TMP_PATH);
    settings_->setModDownloadPath(TMP_PATH);
    sync->addFolder(TORRENT_1, QFileInfo(TMP_PATH).absoluteFilePath(), "@vt5");
    delete sync;
    QThread::sleep(10);
    sync = new LibTorrentApi();

    bool exists = sync->folderExists(TORRENT_1);
    bool rVal = sync->removeFolder(TORRENT_1);
    bool existsAfterDelete = sync->folderExists(TORRENT_1);
    QDir(TMP_PATH).removeRecursively();
    QVERIFY(exists);
    QVERIFY(rVal);
    QVERIFY(!existsAfterDelete);
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

void AfiSyncTest::hash()
{
    beforeDelta();
    QList<QFileInfo> files;
    QDirIterator it("/home/niko/QTProjects/archiver/tests/@ace350",
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        QFileInfo file = it.next();
        if (!file.fileName().endsWith(".pbo"))
            continue;

        files.append(it.next());
    }
    qDebug() << files.size();
    afterDelta();
    QCOMPARE(AHasher::hash(files), QString("BDKZ"));
}

void AfiSyncTest::delta()
{
    beforeDelta();
    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH);

    QDir patchesDir(PATCHES_PATH);
    patchesDir.removeRecursively();
    patchesDir.mkpath(".");
    bool rVal = patcher->delta(MOD_PATH_1, MOD_PATH_2);
    delete patcher;
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::chainDelta()
{
    beforeDelta();
    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH);

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
    //QSKIP("Fatal error when ran with other tests");

    beforeDelta();
    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH);

    QEventLoop loop;
    QObject::connect(patcher, SIGNAL(patched(QString, bool)), &loop, SLOT(quit()));
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
    QString modPath = TMP_PATH + "/1";
    settings_->setModDownloadPath(modPath);
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
    DeltaManager* manager = new DeltaManager(handle_);
    bool rVal = manager->patchAvailable(MOD_NAME_1);
    delete manager;
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::managerPatchAvailableNeg()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaManager* manager = new DeltaManager(handle_);
    bool rVal = manager->patchAvailable("@doesnotexist");
    delete manager;
    afterDelta();
    QVERIFY(!rVal);
}

void AfiSyncTest::managerContains()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaManager manager(handle_);
    QString key = "http://fakekey.org/fake.torrent";
    manager.patch(MOD_NAME_1, key);
    bool rVal = manager.contains(key);
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::managerContainsNeg()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaManager* manager = new DeltaManager(handle_);
    bool rVal = manager->contains("http://doesnotexist.org/no.torrent");
    delete manager;
    afterDelta();
    QVERIFY(!rVal);
}


void AfiSyncTest::managerPatch()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaManager manager(handle_);
    manager.patch("@mod1", "http://fakekey.org/fake.torrent");
    afterDelta();
}

void AfiSyncTest::managerPatchNeg()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaManager manager(handle_);
    manager.patch("@doesnotexist", "http://fakekey.org/fake.torrent");
    afterDelta();
}

void AfiSyncTest::deltaDownloadDownloader()
{
    beforeDelta();
    QString modsPath = TMP_PATH + "/1";
    QDir patchesDir = QDir(modsPath + "/" + DELTA_PATCH_NAME);
    patchesDir.removeRecursively();

    handle_ = createHandle(TORRENT_4, modsPath);
    DeltaDownloader downloader(handle_);
    downloader.downloadPatch("@mod1");
    while (downloader.patchDownloaded("@mod1"))
    {
        QThread::sleep(5);
        qDebug() << "Waiting...";
    }
}

//FileUtils tests

void AfiSyncTest::copy()
{
    FileUtils::copy(SRC_PATH, DST_PATH);
    QDir dstDir(DST_PATH);
    bool rVal = dstDir.exists();
    bool childDirExists = QFileInfo(DST_PATH + "/1").isDir();
    FileUtils::safeRemoveRecursively(dstDir); //Cleanup
    QVERIFY(rVal);
    QVERIFY(childDirExists);
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
    FileUtils::copy(SRC_PATH, DST_PATH);
    FileUtils::copy(SRC_PATH, DST_PATH);
    QDir dstDir(DST_PATH);
    bool rVal = dstDir.exists();
    dstDir.removeRecursively(); //cleanup
    QVERIFY(rVal);
}

void AfiSyncTest::noSuchDir()
{
    QDir(DST_PATH).removeRecursively();
    QString noExist = "noExist";
    FileUtils::copy("noExist", DST_PATH);
    QVERIFY(!QFileInfo(DST_PATH).exists());
}

void AfiSyncTest::simpleCmd()
{
    Console* cmd = new Console();
    QVERIFY(!cmd->runCmd("cd \"no such dir\""));
    delete cmd;
}


//JsonReader Tests

void AfiSyncTest::jsonReaderBasic()
{
    startTest();

    reader_.fillEverything(root_, "repoBasic.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().at(0)->name(), QString("@cz75_nochain_a3"));

    cleanupTest();
}

void AfiSyncTest::jsonReader2Repos()
{
    startTest();

    reader_.fillEverything(root_, "2repos1.json");
    QCOMPARE(root_->childItems().size(), 2);

    cleanupTest();
}

void AfiSyncTest::jsonReaderModUpdate()
{
    startTest();

    reader_.fillEverything(root_, "repoBasic.json");
    reader_.fillEverything(root_, "repoUp1.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 2);
    QCOMPARE(root_->childItems().at(0)->mods().at(1)->name(), QString("@st_nametags"));

    cleanupTest();
}

void AfiSyncTest::jsonReaderModRemove()
{
    startTest();

    reader_.fillEverything(root_, "repoUp1.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 2);
    reader_.fillEverything(root_, "repoBasic.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().at(0)->name(), QString("@cz75_nochain_a3"));

    cleanupTest();
}

void AfiSyncTest::jsonReaderRepoRename()
{
    startTest();

    reader_.fillEverything(root_, "repoBasic.json");
    reader_.fillEverything(root_, "repoRename.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->name(), QString("armafinland.fi Primary 2"));

    cleanupTest();
}

void AfiSyncTest::jsonReaderAddRepo()
{
    startTest();

    reader_.fillEverything(root_, "repoBasic.json");
    reader_.fillEverything(root_, "2repos1.json");
    QCOMPARE(root_->childItems().size(), 2);

    cleanupTest();
}

void AfiSyncTest::jsonReaderRemoveRepo()
{
    startTest();

    reader_.fillEverything(root_, "2repos1.json");
    //Produces getHandle error because fake torrents cannot be downloaded!
    reader_.fillEverything(root_, "repoBasic.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->name(), QString("armafinland.fi Primary"));

    cleanupTest();
}

void AfiSyncTest::jsonReaderMoveMod()
{
    startTest();

    reader_.fillEverything(root_, "2repos1.json");
    QCOMPARE(root_->childItems().at(0)->mods().size(), 2);
    QCOMPARE(root_->childItems().at(1)->mods().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().at(1)->name(), QString("@st_nametags"));
    reader_.fillEverything(root_, "2repos2.json");
    QCOMPARE(root_->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root_->childItems().at(1)->mods().size(), 2);
    QCOMPARE(root_->childItems().at(1)->mods().at(1)->name(), QString("@st_nametags"));

    cleanupTest();
}

//Sync tests

void AfiSyncTest::addRemoveFolder()
{
    startTest();

    sync_->addFolder(TORRENT_1, TMP_PATH, "@vt5");
    bool exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, false);

    cleanupTest();
}

void AfiSyncTest::getFilesUpper()
{
    startTest();

    sync_->addFolder(TORRENT_1, TMP_PATH, "@vt5");
    QSet<QString> files = sync_->folderFilesUpper(TORRENT_1);
    QCOMPARE(files.size(), 3);
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

void AfiSyncTest::getFolderKeys()
{
    startTest();

    sync_->addFolder(TORRENT_1, TMP_PATH, "@vt5");
    QCOMPARE(sync_->folderKeys().size(), 1);
    QCOMPARE(sync_->folderKeys().at(0), TORRENT_1.toLower());
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

void AfiSyncTest::setFolderPaused()
{
    startTest();

    sync_->addFolder(TORRENT_1, TMP_PATH, "@vt5");
    sync_->setFolderPaused(TORRENT_1, true);
    QCOMPARE(sync_->folderPaused(TORRENT_1), true);
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

void AfiSyncTest::getEta()
{
    startTest();

    sync_->addFolder(TORRENT_1, TMP_PATH, "@vt5");
    int eta = sync_->folderEta(TORRENT_1);
    qDebug() << "eta =" << eta;
    QVERIFY(eta > 0);
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
