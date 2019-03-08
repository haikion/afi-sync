#ifndef MOCKSYNCITEM_H
#define MOCKSYNCITEM_H

#include "../app/src/model/interfaces/isyncitem.h"

class MockSyncItem : virtual public ISyncItem
{
public:
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

#endif // MOCKSYNCITEM_H
