#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include "../src/model/jsonreader.h"
#include "../src/model/rootitem.h"
#include "../src/model/treemodel.h"
#include "../src/model/repository.h"
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
};

AfiSyncTest::AfiSyncTest()
{
}

void AfiSyncTest::initTestCase()
{
}

void AfiSyncTest::cleanupTestCase()
{
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

QTEST_MAIN(AfiSyncTest)

#include "tst_afisynctest.moc"
