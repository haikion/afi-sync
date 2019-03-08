
#include <QDir>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "../app/src/model/deletabledetector.h"
#include "../app/src/model/interfaces/irepository.h"
#include "../app/src/model/afisync.h"

using namespace testing;

static const QString NAME_1 = "@test1";
static const QString NAME_2 = "@test2";
static const QString NAME_3 = "@test3";

class DeletableDetectorTest : public Test
{
    void SetUp() override
    {
        // Create mod dirs
        QDir dir;
        dir.mkdir(NAME_1);
        dir.mkdir(NAME_2);
        dir.mkdir(NAME_3);
    }

    void TearDown() override
    {
        QDir dir;
        dir.rmdir(NAME_1);
        dir.rmdir(NAME_2);
        dir.rmdir(NAME_3);
    }
};

class MockSyncItem : virtual public ISyncItem
{
public:
    MockSyncItem() = default;
    MOCK_CONST_METHOD0(name, QString());
    MOCK_METHOD0(ticked, bool());
    MOCK_CONST_METHOD0(active, bool());
    MOCK_METHOD0(statusStr, QString());
    MOCK_CONST_METHOD0(etaStr, QString());
    MOCK_CONST_METHOD0(sizeStr, QString());
    MOCK_METHOD0(optional, bool());
    MOCK_METHOD0(checkboxClicked, void());
    MOCK_METHOD0(check, void());
    MOCK_METHOD0(progressStr, QString());
};

class MockRepository : public MockSyncItem, virtual public IRepository
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(join, void());
    MOCK_CONST_METHOD0(uiMods, QList<ISyncItem*>());
};

TEST_F(DeletableDetectorTest, emptyDirs)
{
    QList<IRepository*> repositories;
    DeletableDetector deletableDetector(".", repositories);

    const QStringList deletableNames = deletableDetector.deletableNames();
    ASSERT_TRUE(deletableNames.contains(NAME_1));
    ASSERT_TRUE(deletableNames.contains(NAME_2));
    ASSERT_TRUE(deletableNames.contains(NAME_3));
    ASSERT_EQ(0, deletableDetector.totalSize());
}

TEST_F(DeletableDetectorTest, twoDeletables)
{
    QList<ISyncItem*> mods;
    MockSyncItem mod;
    EXPECT_CALL(mod, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod, ticked()).WillRepeatedly(Return(true));
    mods.append(&mod);
    MockRepository repo;
    EXPECT_CALL(repo, uiMods()).WillRepeatedly(Return(mods));
    QList<IRepository*> repositories;
    repositories.append(&repo);

    DeletableDetector deletableDetector(".", repositories);
    const QStringList deletableNames = deletableDetector.deletableNames();
    ASSERT_FALSE(deletableNames.contains(NAME_1));
    ASSERT_TRUE(deletableNames.contains(NAME_2));
    ASSERT_TRUE(deletableNames.contains(NAME_3));
    ASSERT_EQ(0, deletableDetector.totalSize());
}

TEST_F(DeletableDetectorTest, ticked)
{
    QList<ISyncItem*> mods;
    MockSyncItem mod1;
    EXPECT_CALL(mod1, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod1, ticked()).WillRepeatedly(Return(true));
    MockSyncItem mod2;
    EXPECT_CALL(mod2, name()).WillRepeatedly(Return(NAME_2));
    EXPECT_CALL(mod2, ticked()).WillRepeatedly(Return(false));
    mods.append(&mod1);
    MockRepository repo;
    EXPECT_CALL(repo, uiMods()).WillRepeatedly(Return(mods));
    QList<IRepository*> repositories;
    repositories.append(&repo);

    DeletableDetector deletableDetector(".", repositories);
    const QStringList deletableNames = deletableDetector.deletableNames();
    ASSERT_FALSE(deletableNames.contains(NAME_1));
    ASSERT_TRUE(deletableNames.contains(NAME_2));
    ASSERT_TRUE(deletableNames.contains(NAME_3));
    ASSERT_EQ(0, deletableDetector.totalSize());
}

TEST_F(DeletableDetectorTest, activeModNames)
{
    QList<ISyncItem*> mods;
    MockSyncItem mod;
    EXPECT_CALL(mod, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod, ticked()).WillRepeatedly(Return(true));
    mods.append(&mod);
    MockRepository repo;
    QList<IRepository*> repositories;
    repositories.append(&repo);
    EXPECT_CALL(repo, uiMods()).WillRepeatedly(Return(mods));

    const QStringList retVal = AfiSync::activeModNames(repositories);
    ASSERT_TRUE(retVal.contains(NAME_1));
}

