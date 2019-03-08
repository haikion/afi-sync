#include <QDir>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>

#include "../app/src/model/deletabledetector.h"
#include "../app/src/model/afisync.h"

#include "mockmod.h"
#include "mockrepository.h"
#include "testconstants.h"

using namespace testing;

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
    QList<IMod*> mods;
    MockMod mod;
    EXPECT_CALL(mod, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod, selected()).WillRepeatedly(Return(true));
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
    QList<IMod*> mods;

    MockMod mod1;
    EXPECT_CALL(mod1, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod1, selected()).WillRepeatedly(Return(true));
    mods.append(&mod1);

    MockMod mod2;
    EXPECT_CALL(mod2, name()).WillRepeatedly(Return(NAME_2));
    EXPECT_CALL(mod2, selected()).WillRepeatedly(Return(false));
    mods.append(&mod2);

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
