#ifndef DELETABLE_H
#define DELETABLE_H

#include <QString>

class Deletable
{
public:
    Deletable(const Deletable& other);
    Deletable(const QString& name, const qint64& size);
    Deletable& operator=(const Deletable& other);

    virtual ~Deletable() = default;

    QString name() const;
    void setName(const QString& name);
    qint64 size() const;
    void setSize(const qint64& size);

private:
    QString name_;
    qint64 size_;
};

#endif // DELETABLE_H
