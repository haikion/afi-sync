#ifndef DEBUG_H
#define DEBUG_H
#include "global.h"

#define DBG qDebug() << Global::runningTime->elapsed() << Q_FUNC_INFO

#endif // DEBUG_H
