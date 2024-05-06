#ifndef MOCKSYNCITEM_H
#define MOCKSYNCITEM_H

#include <gmock/gmock.h>

#include "../app/src/model/interfaces/isyncitem.h"

class MockSyncItem : virtual public ISyncItem
{
public:
    MOCK_METHOD(QString, name, (), (override));
    MOCK_METHOD(QString, statusStr, (), (override));
    MOCK_METHOD(QString, sizeStr, (), (const, override));
    MOCK_METHOD(bool, optional, (), (override));
    MOCK_METHOD(bool, ticked, (), (override));
    MOCK_METHOD(void, checkboxClicked, (), (override));
    MOCK_METHOD(void, check, (), (override));
    MOCK_METHOD(bool, active, (), (const, override));
    MOCK_METHOD(QString, progressStr, (), (override));
};

#endif // MOCKSYNCITEM_H
