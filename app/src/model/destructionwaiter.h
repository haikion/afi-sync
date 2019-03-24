#ifndef DESTRUCTIONWAITER_H
#define DESTRUCTIONWAITER_H
#include <QObject>
#include <QSet>

class DestructionWaiter
{
public:
    DestructionWaiter(QObject* object);
    DestructionWaiter(const QSet<QObject*>& set);
    void wait(int timeout = -1);

private:
    std::atomic<int> counter{0};
    void decrement();
    void init(const QSet<QObject*>& objects);
};

#endif // DESTRUCTIONWAITER_H
