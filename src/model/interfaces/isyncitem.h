#ifndef ISYNCITEM_H
#define ISYNCITEM_H

#include <QList>
#include <QString>

/**
 * @brief UI Representation of Sync Item
 */
class ISyncItem
{
public:
    virtual QString name() const = 0;
    virtual QString statusStr() = 0;
    virtual QString etaStr() const = 0;
    virtual QString sizeStr() const = 0;
    virtual bool optional() = 0;
    virtual bool ticked() const = 0;
    virtual void checkboxClicked() = 0;
    virtual void check() = 0;
};

#endif // ISYNCITEM_H
