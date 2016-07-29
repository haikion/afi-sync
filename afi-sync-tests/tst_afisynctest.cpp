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

const QString TORRENT_1 = "http://mythbox.no-ip.org/torrents/@vt5_1.torrent";
//Delta patching consts
static const QString FILES_PATH = "files";
static const QString TMP_PATH = "temp";
static const QString TORRENT_2 = FILES_PATH  + "/afisync_patches_1.torrent";
static const QString MOD_NAME_1 = "@mod1";
static const QString MOD_PATH_1 = TMP_PATH + "/1/" + MOD_NAME_1;
static const QString MOD_PATH_2 = TMP_PATH + "/2/" + MOD_NAME_1;
static const QString MOD_PATH_3 = TMP_PATH + "/3/" + MOD_NAME_1;
static const QString DELTA_PATCH_NAME = "afisync_patches";
static const QString PATCHES_DIR = TMP_PATH + "/" + DELTA_PATCH_NAME;
//file copy tests
static const QString SRC_PATH = FILES_PATH;
static const QString DST_PATH = TMP_PATH;
static const QString DEEP_PATH = TMP_PATH + "/deeper";


class AfiSyncTest : public QObject
{
    Q_OBJECT

public:
    AfiSyncTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    //JsonReader Tests
    void jsonReaderBasic();
    void jsonReaderModUpdate();
    void jsonReaderModRemove();
    void jsonReaderRepoRename();
    void jsonReaderAddRepo();
    void jsonReaderRemoveRepo();
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
    void chainPatch();
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
    void deltaExtraFileDeletion();
    //FileUtils tests
    void copy();
    void copyDeep();
    void noSuchDir();
    void dstExists();

private:
    LibTorrentApi* sync_;
    TreeModel* model_;
    JsonReader reader_;
    RootItem* root_;
    //Delta patching
    DeltaPatcher* patcher_;
    libtorrent::torrent_handle handle_;
    libtorrent::session* session_;
    SettingsModel* settings_;

    libtorrent::torrent_handle createHandle(const QString& modDownloadPath = TMP_PATH);
};

AfiSyncTest::AfiSyncTest():
   sync_(nullptr),
   model_(nullptr),
   root_(nullptr),
   patcher_(nullptr),
   session_(nullptr),
   settings_(new SettingsModel(this))
{
   handle_ = createHandle();
   settings_->setModDownloadPath(TMP_PATH + "/1");
}

void AfiSyncTest::initTestCase()
{
    if (sync_)
       delete sync_;
    if (model_)
       delete model_;
    if (root_)
       delete root_;

    QDir(Constants::SYNC_SETTINGS_PATH).removeRecursively();
    sync_ = new LibTorrentApi();
    model_ = new TreeModel();
    root_ = new RootItem(model_);
}

void AfiSyncTest::cleanupTestCase()
{
    delete sync_;
    delete model_;
    delete root_;
}

void AfiSyncTest::jsonReaderBasic()
{
    reader_.fillEverything(root_, "repoBasic.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().at(0)->name(), QString("@cz75_nochain_a3"));
}

void AfiSyncTest::jsonReaderModUpdate()
{
    reader_.fillEverything(root_, "repoBasic.json");
    reader_.fillEverything(root_, "repoUp1.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 2);
    QCOMPARE(root_->childItems().at(0)->mods().at(1)->name(), QString("@update"));
}

void AfiSyncTest::jsonReaderModRemove()
{
    reader_.fillEverything(root_, "repoUp1.json");
    reader_.fillEverything(root_, "repoBasic.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root_->childItems().at(0)->mods().at(0)->name(), QString("@cz75_nochain_a3"));
}

void AfiSyncTest::jsonReaderRepoRename()
{
    reader_.fillEverything(root_, "repoBasic.json");
    reader_.fillEverything(root_, "repoRename.json");
    QCOMPARE(root_->childItems().size(), 1);
    QCOMPARE(root_->childItems().at(0)->name(), QString("armafinland.fi Primary2"));
}

void AfiSyncTest::jsonReaderAddRepo()
{
    reader_.fillEverything(root_, "repoBasic.json");
    reader_.fillEverything(root_, "repo2.json");
    QCOMPARE(root_->childItems().size(), 2);
}

void AfiSyncTest::jsonReaderRemoveRepo()
{
    reader_.fillEverything(root_, "repo2.json");
    reader_.fillEverything(root_, "repoBasic.json");
    QCOMPARE(root_->childItems().size(),1);
    QCOMPARE(root_->childItems().at(0)->name(), QString("armafinland.fi Primary"));
}

void AfiSyncTest::addRemoveFolder()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Downloads/ltTest", "@vt5");
    bool exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, false);
}

void AfiSyncTest::getFilesUpper()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest", "@vt5");
    QSet<QString> files = sync_->folderFilesUpper(TORRENT_1);
    QCOMPARE(files.size(), 3);
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::getFolderKeys()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest", "@vt5");
    QCOMPARE(sync_->folderKeys().size(), 1);
    QCOMPARE(sync_->folderKeys().at(0), TORRENT_1.toLower());
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::setFolderPaused()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest", "@vt5");
    sync_->setFolderPaused(TORRENT_1, true);
    QCOMPARE(sync_->folderPaused(TORRENT_1), true);
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::getEta()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest", "@vt5");
    int eta = sync_->folderEta(TORRENT_1);
    qDebug() << "eta =" << eta;
    QVERIFY(eta > 0);
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::saveAndLoad()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Downloads/ltTest", "@vt5");
    delete sync_;
    QThread::sleep(10);
    sync_ = new LibTorrentApi();
    bool exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, false);
}

//LibTorrent Delta Patching Tests

//Creates clean directory and file stucture for delta patch tests.
void AfiSyncTest::beforeDelta()
{
    FileUtils::copy(FILES_PATH, TMP_PATH);
    FileUtils::copy(PATCHES_DIR, TMP_PATH + "/1/" + DELTA_PATCH_NAME);
    FileUtils::copy(PATCHES_DIR, TMP_PATH + "/2/" + DELTA_PATCH_NAME);
    FileUtils::copy(PATCHES_DIR, TMP_PATH + "/3/" + DELTA_PATCH_NAME);
    QDir().mkpath(PATCHES_DIR);
    QDir().mkpath(TMP_PATH);
    patcher_ = new DeltaPatcher(PATCHES_DIR);
}

void AfiSyncTest::afterDelta()
{
    QDir(TMP_PATH).removeRecursively();
    delete patcher_;
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
    QDir patchesDir(PATCHES_DIR);
    patchesDir.removeRecursively();
    patchesDir.mkpath(".");
    bool rVal = patcher_->delta(MOD_PATH_1, MOD_PATH_2);
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::chainDelta()
{
    beforeDelta();
    QDir patchesDir(PATCHES_DIR);
    patchesDir.removeRecursively();
    patchesDir.mkpath(".");
    bool rVal1 = patcher_->delta(MOD_PATH_1, MOD_PATH_2);
    bool rVal2 = patcher_->delta(MOD_PATH_2, MOD_PATH_3);
    afterDelta();
    QVERIFY(rVal1);
    QVERIFY(rVal2);
}

void AfiSyncTest::chainPatch()
{
    beforeDelta();
    QEventLoop loop;
    QObject::connect(patcher_, SIGNAL(patched(QString, bool)), &loop, SLOT(quit()));
    patcher_->patch(MOD_PATH_1);
    qDebug() << "Waiting.. patched signal";
    loop.exec();
    qDebug() << "Patched signal received";
    QString hash1 = AHasher::hash(MOD_PATH_1);
    QString hash2 = AHasher::hash(MOD_PATH_3);
    afterDelta();
    QCOMPARE(hash1, hash2);
}

void AfiSyncTest::torrentName()
{
    beforeDelta();
    QString name = QString::fromStdString(handle_.torrent_file()->name());
    afterDelta();
    QCOMPARE(name, DELTA_PATCH_NAME);
}

void AfiSyncTest::patch()
{
    beforeDelta();
    QEventLoop loop;
    QObject::connect(patcher_, SIGNAL(patched(QString, bool)), &loop, SLOT(quit()));
    patcher_->patch(MOD_PATH_1);
    qDebug() << "Waiting.. patched signal";
    loop.exec();
    qDebug() << "Patched signal received";
    QString hash1 = AHasher::hash(MOD_PATH_1);
    QString hash2 = AHasher::hash(MOD_PATH_3);
    afterDelta();
    QCOMPARE(hash1, hash2);
}

libtorrent::torrent_handle AfiSyncTest::createHandle(const QString& modDownloadPath)
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
    p.ti = boost::make_shared<torrent_info>(TORRENT_2.toStdString(), boost::ref(ec), 0);
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
    return handle;
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
    DeltaManager manager(handle_);
    bool rVal = manager.patchAvailable(MOD_NAME_1);
    afterDelta();
    QVERIFY(rVal);
}

void AfiSyncTest::managerPatchAvailableNeg()
{
    beforeDelta();
    settings_->setModDownloadPath(TMP_PATH + "/1");
    DeltaManager manager(handle_);
    bool rVal = manager.patchAvailable("@doesnotexist");
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
    DeltaManager manager(handle_);
    bool rVal = manager.contains("http://doesnotexist.org/no.torrent");
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

void AfiSyncTest::deltaExtraFileDeletion()
{
    beforeDelta();
    QString modsPath = TMP_PATH + "/1";
    handle_ = createHandle(modsPath);
    QString tmpTorrent = modsPath + "/afisync_patches/afisync_patches_1.torrent";

    settings_->setModDownloadPath(modsPath);
    FileUtils::copy(TORRENT_2, tmpTorrent);
    DeltaManager manager(handle_);
    QEventLoop loop;
    manager.patch("@mod1", "http://fakeurl.com/@mod1_3.torrent");
    connect(&manager, SIGNAL(patched(QString, QString, bool)), &loop, SLOT(quit()));
    loop.exec();
    bool exists = QFile(tmpTorrent).exists();
    afterDelta();
    QVERIFY(!exists);
}

//FileUtils tests

void AfiSyncTest::copy()
{
    FileUtils::copy(SRC_PATH, DST_PATH);
    QDir dstDir(DST_PATH);
    bool rVal = dstDir.exists();
    bool childDirExists = QFileInfo(DST_PATH + "/1").isDir();
    dstDir.removeRecursively(); //cleanup
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

void AfiSyncTest::noSuchDir()
{
    QDir(DST_PATH).removeRecursively();
    QString noExist = "noExist";
    FileUtils::copy("noExist", DST_PATH);
    QVERIFY(!QFileInfo(DST_PATH).exists());
}

void AfiSyncTest::dstExists()
{
    QDir().mkpath(TMP_PATH);
    FileUtils::copy(SRC_PATH, DST_PATH);
    QString name = QFileInfo(DST_PATH).fileName();
    QDir dstDir(DST_PATH + "/" + name);
    bool rVal = dstDir.exists();
    QDir(TMP_PATH).removeRecursively(); //cleanup
    QVERIFY(rVal);
}

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
