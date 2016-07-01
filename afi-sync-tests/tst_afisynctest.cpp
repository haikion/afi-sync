#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QString>
#include "../src/model/jsonreader.h"
#include "../src/model/rootitem.h"
#include "../src/model/treemodel.h"
#include "../src/model/repository.h"
#include "../src/model/global.h"
#include "../src/model/apis/libtorrent/libtorrentapi.h"

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

private:
    LibTorrentApi* sync_;
    static const QString TORRENT_1;
    TreeModel* model_;
    JsonReader reader_;
    RootItem* root_;
};

const QString AfiSyncTest::TORRENT_1 = "http://mythbox.no-ip.org/torrents/@vt5_1.torrent";

AfiSyncTest::AfiSyncTest():
   sync_(nullptr),
   model_(nullptr),
   root_(nullptr)
{
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
    sync_->addFolder(TORRENT_1, "/home/niko/Downloads/ltTest");
    bool exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->folderExists(TORRENT_1);
    QCOMPARE(exists, false);
}

void AfiSyncTest::getFilesUpper()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest");
    QSet<QString> files = sync_->folderFilesUpper(TORRENT_1);
    QCOMPARE(files.size(), 3);
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::getFolderKeys()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest");
    QCOMPARE(sync_->folderKeys().size(), 1);
    QCOMPARE(sync_->folderKeys().at(0), TORRENT_1.toLower());
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::setFolderPaused()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest");
    sync_->setFolderPaused(TORRENT_1, true);
    QCOMPARE(sync_->folderPaused(TORRENT_1), true);
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::getEta()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Download/ltTest");
    int eta = sync_->folderEta(TORRENT_1);
    qDebug() << "eta =" << eta;
    QVERIFY(eta > 0);
    sync_->removeFolder(TORRENT_1);
}

void AfiSyncTest::saveAndLoad()
{
    sync_->addFolder(TORRENT_1, "/home/niko/Downloads/ltTest");
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

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
