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
    while (counter > 0)
    {
        if (seconds > timeout && timeout != -1)
        {
            LOG << "ERROR: Timeout reached.";
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
        counter++;
        QObject::connect(object, &QObject::destroyed, [=] ()
        {
            this->decrement();
        });
    }
}

void DestructionWaiter::decrement()
{
    counter--;
}
