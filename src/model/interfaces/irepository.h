#ifndef IREPOSITORY_H
#define IREPOSITORY_H

#include <QList>
#include "isyncitem.h"

class IRepository : virtual public ISyncItem
{
public:
    virtual ~IRepository() {}

    virtual QList<ISyncItem*> uiMods() const = 0;
    virtual void start() = 0;
    virtual void join() = 0;
};

#endif // IREPOSITORY_H
