#ifndef MOCKMOD_H
#define MOCKMOD_H

#include "../app/src/model/interfaces/imod.h"
#include "mocksyncitem.h"

class MockMod : public MockSyncItem, virtual public IMod
{
public:
    MOCK_METHOD0(selected, bool());
};

#endif // MOCKMOD_H
