#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QStringLiteral>

#include "../app/src/model/afisync.h"

#include "mockmod.h"
#include "mockrepository.h"
#include "testconstants.h"

using namespace Qt::StringLiterals;
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

TEST(AfiSyncTest, getDeltaUrls)
{
    QStringList patches = {u"http://localhost/afisync-tests/torrents/@cba_a3.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s,
                           u"http://localhost/afisync-tests/torrents/@dummy.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s,
                           u"http://localhost/afisync-tests/torrents/@cba_a3.1.7b21612bc886c7e38bc0bf033fb40cb4.7z.torrent"_s};
    auto result = AfiSync::getDeltaUrlsForMod(u"@cba_a3"_s, u"5c668c74b841d239fb6418e978a07fa5"_s, patches);
    QStringList expected = {u"http://localhost/afisync-tests/torrents/@cba_a3.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s,
                            u"http://localhost/afisync-tests/torrents/@cba_a3.1.7b21612bc886c7e38bc0bf033fb40cb4.7z.torrent"_s};
    ASSERT_EQ(expected, result);
}

// Order must remain the same
TEST(AfiSyncTest, getDeltaUrls2)
{
    QStringList patches = {u"http://localhost/afisync-tests/torrents/@cba_a3.1.7b21612bc886c7e38bc0bf033fb40cb4.7z.torrent"_s,
                           u"http://localhost/afisync-tests/torrents/@cba_a3.2.7b21612bc886c4e38bc0bf033fb40cb4.7z.torrent"_s,
                           u"http://localhost/afisync-tests/torrents/@cba_a3.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s,
                           u"http://localhost/afisync-tests/torrents/@dummy.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s};
    auto result = AfiSync::getDeltaUrlsForMod(u"@cba_a3"_s, u"5c668c74b841d239fb6418e978a07fa5"_s, patches);
    QStringList expected = {u"http://localhost/afisync-tests/torrents/@cba_a3.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s,
                            u"http://localhost/afisync-tests/torrents/@cba_a3.1.7b21612bc886c7e38bc0bf033fb40cb4.7z.torrent"_s,
                            u"http://localhost/afisync-tests/torrents/@cba_a3.2.7b21612bc886c4e38bc0bf033fb40cb4.7z.torrent"_s};
    ASSERT_EQ(expected, result);
}

TEST(AfiSyncTest, filterDeprecatedDeltaPatches)
{
    QStringList patches = {u"http://localhost/afisync-tests/torrents/@cba_a3.0.5c668c74b841d239fb6418e978a07fa5.7z.torrent"_s};
    QStringList files = {u"@cba_a3.0.5c668c74b841d239fb6418e978a07fa5.7z"_s,
                         u"@cba_a3.1.7b21612bc886c7e38bc0bf033fb40cb4.7z"_s};
    QStringList result = AfiSync::filterDeprecatedPatches(files, patches);
    ASSERT_EQ(result, QStringList{u"@cba_a3.1.7b21612bc886c7e38bc0bf033fb40cb4.7z"_s});
}
