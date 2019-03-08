#include "deletable.h"

Deletable::Deletable(const Deletable& other):
    Deletable(other.name(), other.size())
{
}

Deletable::Deletable(const QString& name, const qint64& size):
    name_(name),
    size_(size)
{
}

Deletable& Deletable::operator=(const Deletable& other)
{
    name_ = other.name();
    size_ = other.size();
    return *this;
}

QString Deletable::name() const
{
    return name_;
}

void Deletable::setName(const QString& name)
{
    name_ = name;
}

qint64 Deletable::size() const
{
    return size_;
}

void Deletable::setSize(const qint64& size)
{
    size_ = size;
}
