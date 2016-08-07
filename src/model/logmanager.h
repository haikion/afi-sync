#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QString>
#include "console.h"
#include "szip.h"

class LogManager
{
public:    
    bool rotateLogs();

private:
    static const int MAX_LOGS_SIZE;
    static const QString SZIP_EXECUTABLE;

    Szip szip_;
};

#endif // LOGMANAGER_H
