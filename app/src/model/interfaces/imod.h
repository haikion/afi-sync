#ifndef IMOD_H
#define IMOD_H

#include "isyncitem.h"

class IMod : virtual public ISyncItem
{
public:
	IMod() = default;
    ~IMod() override = default;

    [[nodiscard]] virtual bool selected() = 0;
    Q_DISABLE_COPY(IMod)
};

#endif // IMOD_H
