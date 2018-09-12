#ifndef IBANDWIDTHMETER_H
#define IBANDWIDTHMETER_H

#include <QString>

class IBandwidthMeter
{
public:
    virtual QString uploadStr() = 0;
    virtual QString downloadStr() = 0;
};

#endif // IBANDWIDTHMETER_H
