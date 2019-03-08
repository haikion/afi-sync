#ifndef MOCKREPOSITORY_H
#define MOCKREPOSITORY_H

#include "../app/src/model/interfaces/irepository.h"
#include "mocksyncitem.h"

class MockRepository : public MockSyncItem, virtual public IRepository
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(join, void());
    MOCK_CONST_METHOD0(uiMods, QList<IMod*>());
};


#endif // MOCKREPOSITORY_H
