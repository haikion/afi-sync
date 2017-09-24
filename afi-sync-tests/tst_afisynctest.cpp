/*
 * Unit tests for AFISync
 * Make sure workingDirectory is defined as working directory.
 *
 * TODO: Create tests for delta patch generation when files are identical
 */

#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QProcess>

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
#include "../src/model/modadapter.h"
#include "../src/model/bissignaturechecker.h"
#include "../src/model/sigcheckprocess.h"

//File paths etc..
static const QString PATCHING_FILES_PATH = "patching";
static const QString MODS_PATH = "mods";
static const QString TMP_PATH = "temp";
static const QString TORRENT_1 = "http://mythbox.pwnz.org/torrents/@vt5_1.torrent";
static const QString TORRENT_2 = PATCHING_FILES_PATH  + "/afisync_patches_1.torrent";
static const QString TORRENT_4 = "http://mythbox.pwnz.org/torrents/afisync_patches_1.torrent";
static const QString TORRENT_PATH_1 = "torrents/@vt5_1.torrent";
static const QString MOD1_3_NAME = "@mod1_3.torrent";
static const QString TORRENT_PATH_MOD1_3 = PATCHING_FILES_PATH + "/" + MOD1_3_NAME;
static const QString TORRENT_PATH_MOD1_3_TMP = TMP_PATH + "/" + MOD1_3_NAME;
static const QString MOD_NAME_1 = "@mod1";
static const QString MOD_PATH_1 = TMP_PATH + "/1/" + MOD_NAME_1;
static const QString MOD_PATH_2 = TMP_PATH + "/2/" + MOD_NAME_1;
static const QString MOD_PATH_3 = TMP_PATH + "/3/" + MOD_NAME_1;
static const QString MOD_PATH_4 = TMP_PATH + "/sameHash3/" + MOD_NAME_1;
static const QString MOD_PATH_FILES_MISSING = MODS_PATH + "/@signatureCheckModFilesMissing";
static const QString MOD_PATH_CORRUPTED_PBO = MODS_PATH + "/@signatureCheckModCorruptedPbo";
static const QString MOD_PATH_VALID_CHECK = MODS_PATH + "/@signatureCheckMod";
static const QString MOD_PATH_KEYS_MISSING = MODS_PATH + "/@signatureCheckModKeysMissing";
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
    void checkFilesExist();
    void checkFilesExistNeg();

    //LibTorrent API tests
    void saveAndLoad();
    void getEta();
    void setFolderPaused();
    void getFolderKeys();
    void getFilesUpper();
    void addRemoveFolder();
    void addRemoveFolderKey();

    //Mod tests
    void modFilesRemoved();

    //Repo tests
    void repoTickedFalseDefault();
    void repoCheckboxClicked();

    //Size tests
    void sizeBasic();
    void repoSize();
    void noSize();
    void jsonSize();
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
    void managerContains();
    void managerContainsNeg();
    void managerPatch();
    void managerPatchNeg();
    void deltaNoPeers();

    //JsonReader Tests
    void jsonReaderBasic();
    void jsonReader2Repos();
    void jsonReaderModUpdate();
    void jsonReaderModRemove();
    void jsonReaderRepoRename();
    void jsonReaderAddRepo();
    void jsonReaderRemoveRepo();
    void jsonReaderMoveMod();

    //Bis checker tests
    void bisCheck();
    void bisCheckCorruptedPbo();
    void bisCheckFilesMissing();
    void bisCheckModKeysMissing();
    void bisCheckModDoubleSignature();
    void bisCheckQueue();

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
   settings_(new SettingsModel())
{
   handle_ = createHandle();
   settings_->setModDownloadPath(TMP_PATH + "/1");
}

//Helper functions

void AfiSyncTest::startTest()
{
    if (!model_)
    {
        QDir(SettingsModel::syncSettingsPath()).removeRecursively();
        model_ = new TreeModel();
        root_ = model_->rootItem();
        sync_ = root_->sync();
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

//Creates model, adds folder, deletes model, creates model again. Added folder should still exist.
void AfiSyncTest::saveAndLoad()
{
    ISync* sync = new LibTorrentApi();

    QDir().mkpath(TMP_PATH);
    settings_->setModDownloadPath(TMP_PATH);
    sync->addFolder(TORRENT_PATH_1, "@vt5");
    delete sync;
    QThread::sleep(10);
    sync = new LibTorrentApi();

    bool exists = sync->folderExists(TORRENT_PATH_1);
    bool rVal = sync->removeFolder(TORRENT_PATH_1);
    bool existsAfterDelete = sync->folderExists(TORRENT_PATH_1);
    FileUtils::safeRemoveRecursively(TMP_PATH);
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

void AfiSyncTest::deltaIdentical()
{
    beforeDelta();

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH);
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

    DeltaPatcher* patcher = new DeltaPatcher(PATCHES_PATH);
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

//Tests that the folder status is NO_PEERS
//when there are no peers for afisync_patches torrent.
void AfiSyncTest::deltaNoPeers()
{
    settings_->setDeltaPatchingEnabled(true);
    beforeDelta();
    startTest();
    Global::sync->setDeltaUpdatesFolder(TMP_PATH + "/afisync_patches_nopeers.torrent");
    Global::sync->enableDeltaUpdates();

    QString modsPath = TMP_PATH + "/1";
    settings_->setModDownloadPath(modsPath);
    FileUtils::safeRemoveRecursively(modsPath + "/" + Constants::DELTA_PATCHES_NAME);

    Repository* repo = new Repository("repo", "dummy", 21221, "", root_);
    BisSignatureChecker checker;
    Mod* mod = new Mod("@mod1", TMP_PATH + "/@mod1_1.torrent", &checker);
    repo->checkboxClicked();
    new ModAdapter(mod, repo, false, 0);
    QThread::sleep(3); //Wait for workerThread to finish
    QCOMPARE(mod->status(), SyncStatus::NO_PEERS);
    mod->deleteLater();

    cleanupTest();
    afterDelta();
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

void AfiSyncTest::checkFilesExist()
{
    QSet<QString> filePaths;
    filePaths.insert(QFileInfo(MOD_PATH_FILES_MISSING + "/KeYs/ace_3.9.2.18.bikey").absoluteFilePath().toUpper());
    QVERIFY(FileUtils::filesExistCi(filePaths));
}

void AfiSyncTest::checkFilesExistNeg()
{
    QSet<QString> filePaths;
    filePaths.insert("/some/rnd/file.txt");
    QVERIFY(!FileUtils::filesExistCi(filePaths));
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

void AfiSyncTest::bisCheck()
{
    BisSignatureChecker checker;
    QSet<QString> filePaths;
    filePaths.insert(MOD_PATH_VALID_CHECK + "/addons/ace_norearm.pbo");
    filePaths.insert(MOD_PATH_VALID_CHECK + "/addons/ace_norearm.pbo.ace_3.9.2.18-5f0e6b71.bisign");
    filePaths.insert(MOD_PATH_VALID_CHECK + "/keys/ace_3.9.2.18.bikey");
    QSharedPointer<SigCheckProcess> process = checker.check(MOD_PATH_VALID_CHECK, filePaths);
    QEventLoop loop;
    connect(process.data(), SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(SigCheckProcess::CheckStatus::SUCCESS, process->checkStatus());
}

void AfiSyncTest::bisCheckCorruptedPbo()
{
    BisSignatureChecker checker;
    QSet<QString> filePaths;
    filePaths.insert(MOD_PATH_VALID_CHECK + "/addons/ace_norearm.pbo");
    filePaths.insert(MOD_PATH_VALID_CHECK + "/addons/ace_norearm.pbo.ace_3.9.2.18-5f0e6b71.bisign");
    filePaths.insert(MOD_PATH_VALID_CHECK + "/keys/ace_3.9.2.18.bikey");
    QSharedPointer<SigCheckProcess> process = checker.check(MOD_PATH_CORRUPTED_PBO, filePaths);
    QEventLoop loop;
    connect(process.data(), SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();
    QCOMPARE(SigCheckProcess::CheckStatus::SUCCESS, process->checkStatus());
}

//DSCheckSignatures.exe passes even when there are not addons.
//This tests wether file existance test works
void AfiSyncTest::bisCheckFilesMissing()
{
    BisSignatureChecker checker;
    QSet<QString> filePaths;
    filePaths.insert(MOD_PATH_FILES_MISSING + "/addons/ace_norearm.pbo");
    filePaths.insert(MOD_PATH_FILES_MISSING + "/addons/ace_norearm.pbo.ace_3.9.2.18-5f0e6b71.bisign");
    filePaths.insert(MOD_PATH_FILES_MISSING + "/keys/ace_3.9.2.18.bikey");
    QSharedPointer<SigCheckProcess> process = checker.check(MOD_PATH_FILES_MISSING, filePaths);

    QCOMPARE(SigCheckProcess::CheckStatus::FAILURE, process->checkStatus());
}

void AfiSyncTest::bisCheckModKeysMissing()
{
    BisSignatureChecker checker;
    QSet<QString> filePaths;
    filePaths.insert(MOD_PATH_KEYS_MISSING + "/addons/ace_norearm.pbo");
    filePaths.insert(MOD_PATH_KEYS_MISSING + "/addons/ace_norearm.pbo.ace_3.9.2.18-5f0e6b71.bisign");
    QSharedPointer<SigCheckProcess> process = checker.check(MOD_PATH_KEYS_MISSING, filePaths);

    QEventLoop loop;
    connect(process.data(), SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(SigCheckProcess::CheckStatus::FAILURE, process->checkStatus());
}

void AfiSyncTest::bisCheckModDoubleSignature()
{
    BisSignatureChecker checker;
    QSet<QString> filePaths;
    filePaths.insert(MOD_PATH_KEYS_MISSING + "/addons/ace_norearm.pbo");
    filePaths.insert(MOD_PATH_KEYS_MISSING + "/addons/ace_norearm.pbo.ace_3.10.2.22-23cf08c2.bisign");
    filePaths.insert(MOD_PATH_KEYS_MISSING + "/keys/ace_3.9.2.18.bikey");
    QSharedPointer<SigCheckProcess> process = checker.check(MOD_PATH_KEYS_MISSING, filePaths);

    QEventLoop loop;
    connect(process.data(), SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();

    QCOMPARE(SigCheckProcess::CheckStatus::FAILURE, process->checkStatus());
}

void AfiSyncTest::bisCheckQueue()
{
    BisSignatureChecker* checker = new BisSignatureChecker();
    QSet<QString> filePaths;
    filePaths.insert(MOD_PATH_VALID_CHECK + "/addons/ace_norearm.pbo");
    filePaths.insert(MOD_PATH_VALID_CHECK + "/addons/ace_norearm.pbo.ace_3.9.2.18-5f0e6b71.bisign");
    filePaths.insert(MOD_PATH_VALID_CHECK + "/keys/ace_3.9.2.18.bikey");
    for (int i = 0; i < 20; ++i)
    {
        checker->check(MOD_PATH_VALID_CHECK, filePaths);
    }
    QEventLoop loop;
    connect(checker, SIGNAL(checkDone()), &loop, SLOT(quit()));
    loop.exec();
}

//Sync tests

void AfiSyncTest::addRemoveFolder()
{
    startTest();

    sync_->addFolder(TORRENT_1, "@vt5");
    bool exists = sync_->folderExists(TORRENT_1);
    int counter = 0;
    //Wait 1 sec for async call to finish
    while (!exists && counter < 10)
    {
        ++counter;
        exists = sync_->folderExists(TORRENT_1);
        QThread::msleep(100);
    }

    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, false);

    cleanupTest();
}

void AfiSyncTest::addRemoveFolderKey()
{
    ISync* sync = new LibTorrentApi();

    sync->addFolder(TORRENT_PATH_1, "@vt5");
    bool exists = sync->folderExists(TORRENT_PATH_1);
    QCOMPARE(exists, true);
    bool rVal = sync->removeFolder(TORRENT_PATH_1);
    QCOMPARE(rVal, true);
    exists = sync->folderExists(TORRENT_PATH_1);
    QCOMPARE(exists, false);

    delete sync;
}

//Mod tests

//Tests if files get removed from mod directory
//if that was the only change done.
void AfiSyncTest::modFilesRemoved()
{
    startTest();
    FileUtils::copy("mods", TMP_PATH);
    settings_->setModDownloadPath(TMP_PATH + "/1extraFile");
    Repository* repo = new Repository("dummy2", "address", 1234, "", root_);
    BisSignatureChecker checker;
    Mod* mod = new Mod("@mod1", TORRENT_PATH_MOD1_3, &checker);
    ModAdapter* adp = new ModAdapter(mod, repo, false, 0);
    Q_UNUSED(adp);
    repo->checkboxClicked();
    QSet<QString> readys;
    readys.insert(SyncStatus::READY);
    readys.insert(SyncStatus::READY_PAUSED);
    while (!readys.contains(mod->status()))
    {
        DBG << "Wating... mod status =" << mod->status();
        QThread::sleep(1); //Wait for check to finish
    }
    bool extraFileExists = QFileInfo(TMP_PATH + "/1extraFile/@mod1/extraFile.txt").exists();
    FileUtils::safeRemoveRecursively(TMP_PATH);
    QVERIFY(!extraFileExists);
    cleanupTest();
}

//Repository tests

void AfiSyncTest::repoTickedFalseDefault()
{
    cleanupTest();
    startTest();

    Repository* repo = new Repository("dummy3", "address", 1234, "", root_);
    QVERIFY(!repo->ticked());

    cleanupTest();
}

void AfiSyncTest::repoCheckboxClicked()
{
    startTest();

    Repository* repo = new Repository("dummy4", "address", 1234, "", root_);
    repo->checkboxClicked();
    QVERIFY(repo->ticked());

    cleanupTest();
}

//Size tests

void AfiSyncTest::sizeBasic()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    mod->setFileSize(quint64(1000));
    QVERIFY(mod->fileSize() == quint64(1000));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::repoSize()
{
    startTest();

    Repository* repo = new Repository("name", "address", 1234, "password", root_);
    BisSignatureChecker checker;
    Mod* mod1 = new Mod("@vt5", TORRENT_1, &checker);
    mod1->setFileSize(quint64(1000));
    Mod* mod2 = new Mod("@vt5", TORRENT_1, &checker);
    mod2->setFileSize(quint64(3000));
    new ModAdapter(mod1, repo, false, 0);
    new ModAdapter(mod2, repo, false, 1);

    QVERIFY(repo->fileSize() == quint64(4000));

    cleanupTest();
}

void AfiSyncTest::noSize()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    QVERIFY(mod->fileSize() == 0);
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::jsonSize()
{
    startTest();

    reader_.fillEverything(root_, "repoBasic.json");
    QVERIFY(root_->childItems().at(0)->mods().at(0)->fileSize() == 1000);

    cleanupTest();
}

void AfiSyncTest::sizeStringB()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    mod->setFileSize(quint64(1000));
    QCOMPARE(mod->fileSizeText(), QString("1000.00 B"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringMB()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    mod->setFileSize(quint64(21309));
    QCOMPARE(mod->fileSizeText(), QString("20.81 kB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringGB()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    mod->setFileSize(quint64(3376414));
    QCOMPARE(mod->fileSizeText(), QString("3.22 MB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeStringGB2()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    mod->setFileSize(quint64(33764140));
    QCOMPARE(mod->fileSizeText(), QString("32.20 MB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeString0()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    QCOMPARE(mod->fileSizeText(), QString("??.?? MB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::sizeOverflow()
{
    startTest();

    BisSignatureChecker checker;
    Mod* mod = new Mod("@vt5", TORRENT_1, &checker);
    mod->setFileSize(quint64(13770848165));
    QCOMPARE(mod->fileSizeText(), QString("12.83 GB"));
    mod->deleteLater();

    cleanupTest();
}

void AfiSyncTest::getFilesUpper()
{
    startTest();

    sync_->addFolder(TORRENT_1, "@vt5");
    QSet<QString> files = sync_->folderFilesUpper(TORRENT_1);
    QCOMPARE(files.size(), 3);
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

void AfiSyncTest::getFolderKeys()
{
    startTest();

    sync_->addFolder(TORRENT_1, "@vt5");
    QCOMPARE(sync_->folderKeys().size(), 1);
    QCOMPARE(sync_->folderKeys().at(0), TORRENT_1.toLower());
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

void AfiSyncTest::setFolderPaused()
{
    startTest();

    sync_->addFolder(TORRENT_1, "@vt5");
    sync_->setFolderPaused(TORRENT_1, true);
    QCOMPARE(sync_->folderPaused(TORRENT_1), true);
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

void AfiSyncTest::getEta()
{
    startTest();

    sync_->addFolder(TORRENT_1, "@vt5");
    int eta = sync_->folderEta(TORRENT_1);
    qDebug() << "eta =" << eta;
    QVERIFY(eta > 0);
    sync_->removeFolder(TORRENT_1);

    cleanupTest();
}

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
