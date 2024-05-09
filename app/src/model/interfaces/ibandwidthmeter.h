#ifndef IBANDWIDTHMETER_H
#define IBANDWIDTHMETER_H

#include <QString>

class IBandwidthMeter
{
public:
	IBandwidthMeter() = default;
    virtual ~IBandwidthMeter() = default;

    [[nodiscard]] virtual QString uploadStr() const = 0;
    [[nodiscard]] virtual QString downloadStr() const = 0;

    Q_DISABLE_COPY(IBandwidthMeter)
};

#endif // IBANDWIDTHMETER_H
