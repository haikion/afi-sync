/*
 * Case insensite hash table. Converts all inserted values to lower case.
 */
#ifndef CIHASH_H
#define CIHASH_H

#include <QObject>
#include <QString>
#include <QHash>

template<class T> class CiHash : public QHash<QString, T>
{
public:
    typedef typename QHash<QString, T>::iterator CiHashIt;

    void insert(const QString& key, const T& value)
    {
        QString lowerKey = key.toLower();
        QHash<QString, T>::insert(lowerKey, value);
    }

    int remove(const QString& key)
    {
        QString lowerKey = key.toLower();
        return QHash<QString, T>::remove(lowerKey);
    }

    bool contains(const QString& key) const
    {
        return QHash<QString, T>::contains(key.toLower());
    }

    //FixMe: Figure out why this cannot be const
    CiHashIt find(const QString& key)
    {
        return QHash<QString, T>::find(key.toLower());
    }
};

#endif // CIHASH_H
