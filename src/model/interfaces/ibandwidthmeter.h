#ifndef IBANDWIDTHMETER_H
#define IBANDWIDTHMETER_H

#include <QString>

class IBandwidthMeter
{
public:
    virtual QString uploadStr() const = 0;
    virtual QString downloadStr() const = 0;
};

#endif // IBANDWIDTHMETER_H
