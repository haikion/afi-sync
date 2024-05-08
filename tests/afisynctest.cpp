#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QStringLiteral>

#include "../app/src/model/afisync.h"

using namespace Qt::StringLiterals;
using namespace testing;

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
