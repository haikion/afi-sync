#ifndef CUSTOMDEBUG_H
#define CUSTOMDEBUG_H

#include <QDebug>

QDebug debug()
{
    QDebug db = qDebug();
    db  << Q_FUNC_INFO;
    return db;
}

#endif // CUSTOMDEBUG_H
