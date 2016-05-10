#ifndef DEBUG_H
#define DEBUG_H
#include "global.h"
#include "runningtime.h"

#define DBG qDebug() << runningTimeMs() << Q_FUNC_INFO

#endif // DEBUG_H
