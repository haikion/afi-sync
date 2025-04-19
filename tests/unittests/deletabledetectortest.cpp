#include <QDir>
#include <QStringLiteral>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "model/deletabledetector.h"
#include "testconstants.h"

using namespace Qt::StringLiterals;
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
    DeletableDetector deletableDetector(u"."_s, repositories);

    const QStringList deletableNames = deletableDetector.deletableNames();
    ASSERT_TRUE(deletableNames.contains(NAME_1));
    ASSERT_TRUE(deletableNames.contains(NAME_2));
    ASSERT_TRUE(deletableNames.contains(NAME_3));
    ASSERT_EQ(0, deletableDetector.totalSize());
}
