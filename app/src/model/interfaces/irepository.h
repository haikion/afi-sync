#ifndef IREPOSITORY_H
#define IREPOSITORY_H

#include <QList>
#include <QSharedPointer>

#include "isyncitem.h"

class ModAdapter;

class IRepository : virtual public ISyncItem
{
public:
	IRepository() = default;
    ~IRepository() override = default;

    [[nodiscard]] virtual const QList<QSharedPointer<ModAdapter>>& modAdapters() const = 0;
    virtual void start() = 0;
    virtual void join() = 0;

    Q_DISABLE_COPY(IRepository)
};

#endif // IREPOSITORY_H
