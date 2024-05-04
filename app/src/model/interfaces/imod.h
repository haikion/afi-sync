#ifndef IMOD_H
#define IMOD_H

#include "isyncitem.h"

class IMod : virtual public ISyncItem
{
public:
    virtual ~IMod() = default;
    [[nodiscard]] virtual bool selected() = 0;
};

#endif // IMOD_H
