/*
 * Case insensite hash table. Matches regardless of the case.
 * TODO: Move to  hoxlib
 */
#ifndef CIHASH_H
#define CIHASH_H

#include <QObject>
#include <QString>
#include <QStringList>
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

    void removeAll(const QStringList& keys)
    {
        for (const QString& key : keys)
        {
            remove(key);
        }
    }

    bool contains(const QString& key) const
    {
        return QHash<QString, T>::contains(key.toLower());
    }

    CiHashIt find(const QString& key)
    {
        return QHash<QString, T>::find(key.toLower());
    }
};

#endif // CIHASH_H
