#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../app/src/model/afisync.h"

#include "mockmod.h"
#include "mockrepository.h"
#include "testconstants.h"

using namespace testing;

TEST(AfiSyncTest, activeModNames)
{
    QList<IMod*> mods;
    MockMod mod;
    EXPECT_CALL(mod, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod, selected()).WillRepeatedly(Return(true));
    mods.append(&mod);
    MockRepository repo;
    QList<IRepository*> repositories;
    repositories.append(&repo);
    EXPECT_CALL(repo, uiMods()).WillRepeatedly(Return(mods));

    const QStringList retVal = AfiSync::activeModNames(repositories);
    ASSERT_TRUE(retVal.contains(NAME_1));
}

TEST(AfiSyncTest, twoRepos)
{
    QList<IMod*> mods;
    MockMod mod;
    EXPECT_CALL(mod, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod, selected()).WillRepeatedly(Return(true));
    mods.append(&mod);
    MockRepository repo;
    EXPECT_CALL(repo, uiMods()).WillRepeatedly(Return(mods));
    MockRepository repo2;
    EXPECT_CALL(repo2, uiMods()).WillRepeatedly(Return(mods));
    QList<IRepository*> repositories;
    repositories.append(&repo);
    repositories.append(&repo2);

    const QStringList retVal = AfiSync::activeModNames(repositories);
    ASSERT_EQ(1, retVal.size());
}

TEST(AfiSyncTest, twoRepos2)
{
    QList<IMod*> mods;
    MockMod mod;
    EXPECT_CALL(mod, name()).WillRepeatedly(Return(NAME_1));
    EXPECT_CALL(mod, selected()).WillRepeatedly(Return(true));
    mods.append(&mod);
    MockRepository repo;
    EXPECT_CALL(repo, uiMods()).WillRepeatedly(Return(mods));
    MockRepository repo2;
    EXPECT_CALL(repo2, uiMods()).WillRepeatedly(Return(mods));
    QList<IRepository*> repositories;
    repositories.append(&repo);
    repositories.append(&repo2);

    const QStringList retVal = AfiSync::activeModNames(repositories);
    ASSERT_EQ(1, retVal.size());
}
