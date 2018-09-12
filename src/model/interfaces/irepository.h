#ifndef IREPOSITORY_H
#define IREPOSITORY_H

#include <QList>
#include "isyncitem.h"

class IRepository : public ISyncItem
{
public:
    virtual QList<ISyncItem*> mods() = 0;
    virtual void start() = 0;
    virtual void join() = 0;
};

#endif // IREPOSITORY_H
