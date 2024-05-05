#include <QDebug>
#include <QThread>
#include "afisynclogger.h"
#include "destructionwaiter.h"

DestructionWaiter::DestructionWaiter(QObject* object)
{
    QSet<QObject*> set;
    set.insert(object);
    init(set);
}

DestructionWaiter::DestructionWaiter(const QSet<QObject*>& objects)
{
    init(objects);
}

void DestructionWaiter::wait(int timeout)
{
    int seconds = 0;
    while (counter_ > 0)
    {
        if (seconds > timeout && timeout != -1)
        {
            LOG_ERROR << "Timeout reached.";
            break;
        }
        QThread::sleep(1);
        seconds++;
    }
}

void DestructionWaiter::init(const QSet<QObject*>& objects)
{
    for (QObject* object : objects)
    {
        counter_++;
        // clazy:skip
        QObject::connect(object, &QObject::destroyed, [=] ()
        {
            counter_--;
        });
    }
}
