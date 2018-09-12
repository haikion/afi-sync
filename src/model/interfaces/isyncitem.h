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
    virtual QString name() = 0;
    virtual QString statusStr() = 0;
    virtual QString etaStr() = 0;
    virtual QString sizeStr() = 0;
    virtual bool isOptional() = 0;
    virtual bool checked() = 0;
    virtual void setChecked(bool value) = 0;
    virtual void reCheck() = 0;
};

#endif // ISYNCITEM_H
