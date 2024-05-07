#ifndef IREPOSITORY_H
#define IREPOSITORY_H

#include <QList>
#include "imod.h"

class IRepository : virtual public ISyncItem
{
public:
    ~IRepository() override = default;
    [[nodiscard]] virtual QList<IMod*> uiMods() const = 0; // TODO: Rename
    virtual void start() = 0;
    virtual void join() = 0;
};

#endif // IREPOSITORY_H
