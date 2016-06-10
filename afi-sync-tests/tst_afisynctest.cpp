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
};

const QString AfiSyncTest::TORRENT_1 = "http://mythbox.no-ip.org/torrents/@vt5_1.torrent";

AfiSyncTest::AfiSyncTest()
{
}

void AfiSyncTest::initTestCase()
{
    QDir(Constants::SYNC_SETTINGS_PATH).removeRecursively();
    sync_ = new LibTorrentApi();
}

void AfiSyncTest::cleanupTestCase()
{
    delete sync_;
}

void AfiSyncTest::jsonReaderBasic()
{
    TreeModel* model = new TreeModel("", "", 12345);
    RootItem* root = new RootItem(model);
    JsonReader::fillEverything(root, "repoBasic.json");
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root->childItems().at(0)->mods().at(0)->name(), QString("@cz75_nochain_a3"));
    delete model;
    delete root;
}

void AfiSyncTest::jsonReaderModUpdate()
{
    TreeModel* model = new TreeModel("", "", 12345);
    RootItem* root = new RootItem(model);
    JsonReader::fillEverything(root, "repoBasic.json");
    JsonReader::fillEverything(root, "repoUp1.json");
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0)->mods().size(), 2);
    QCOMPARE(root->childItems().at(0)->mods().at(1)->name(), QString("@update"));
    delete model;
    delete root;
}

void AfiSyncTest::jsonReaderModRemove()
{
    TreeModel* model = new TreeModel("", "", 12345);
    RootItem* root = new RootItem(model);
    JsonReader::fillEverything(root, "repoUp1.json");
    JsonReader::fillEverything(root, "repoBasic.json");
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0)->mods().size(), 1);
    QCOMPARE(root->childItems().at(0)->mods().at(0)->name(), QString("@cz75_nochain_a3"));
    delete model;
    delete root;
}

void AfiSyncTest::jsonReaderRepoRename()
{
    TreeModel* model = new TreeModel("", "", 12345);
    RootItem* root = new RootItem(model);
    JsonReader::fillEverything(root, "repoBasic.json");
    JsonReader::fillEverything(root, "repoRename.json");
    QCOMPARE(root->childItems().size(), 1);
    QCOMPARE(root->childItems().at(0)->name(), QString("armafinland.fi Primary2"));
    delete model;
    delete root;
}

void AfiSyncTest::jsonReaderAddRepo()
{
    TreeModel* model = new TreeModel("", "", 12345);
    RootItem* root = new RootItem(model);
    JsonReader::fillEverything(root, "repoBasic.json");
    JsonReader::fillEverything(root, "repo2.json");
    QCOMPARE(root->childItems().size(), 2);
    delete model;
    delete root;
}

void AfiSyncTest::jsonReaderRemoveRepo()
{
    TreeModel* model = new TreeModel("", "", 12345);
    RootItem* root = new RootItem(model);
    JsonReader::fillEverything(root, "repo2.json");
    JsonReader::fillEverything(root, "repoBasic.json");
    QCOMPARE(root->childItems().size(),1);
    QCOMPARE(root->childItems().at(0)->name(), QString("armafinland.fi Primary"));
    delete model;
    delete root;
}

void AfiSyncTest::addRemoveFolder()
{
    sync_->addFolder("/home/niko/Downloads/ltTest", TORRENT_1);
    bool exists = sync_->exists(TORRENT_1);
    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder2(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->exists(TORRENT_1);
    QCOMPARE(exists, false);
}

void AfiSyncTest::getFilesUpper()
{
    sync_->addFolder("/home/niko/Download/ltTest", TORRENT_1);
    QSet<QString> files = sync_->getFilesUpper(TORRENT_1);
    QCOMPARE(files.size(), 3);
    sync_->removeFolder2(TORRENT_1);
}

void AfiSyncTest::getFolderKeys()
{
    sync_->addFolder("/home/niko/Download/ltTest", TORRENT_1);
    QCOMPARE(sync_->getFolderKeys().size(), 1);
    QCOMPARE(sync_->getFolderKeys().at(0), TORRENT_1.toLower());
    sync_->removeFolder2(TORRENT_1);
}

void AfiSyncTest::setFolderPaused()
{
    sync_->addFolder("/home/niko/Download/ltTest", TORRENT_1);
    sync_->setFolderPaused(TORRENT_1, true);
    QCOMPARE(sync_->paused(TORRENT_1), true);
    sync_->removeFolder2(TORRENT_1);
}

void AfiSyncTest::getEta()
{
    sync_->addFolder("/home/niko/Download/ltTest", TORRENT_1);
    int eta = sync_->getFolderEta(TORRENT_1);
    qDebug() << "eta =" << eta;
    QVERIFY(eta > 0);
    sync_->removeFolder2(TORRENT_1);
}

void AfiSyncTest::saveAndLoad()
{
    sync_->addFolder("/home/niko/Downloads/ltTest", TORRENT_1);
    QThread::sleep(10);
    delete sync_;
    sync_ = new LibTorrentApi();
    bool exists = sync_->exists(TORRENT_1);
    QCOMPARE(exists, true);
    bool rVal = sync_->removeFolder2(TORRENT_1);
    QCOMPARE(rVal, true);
    exists = sync_->exists(TORRENT_1);
    QCOMPARE(exists, false);
}

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
