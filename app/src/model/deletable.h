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

    [[nodiscard]] QString name() const;
    [[nodiscard]] qint64 size() const;

private:
    QString name_;
    qint64 size_;
};

#endif // DELETABLE_H
