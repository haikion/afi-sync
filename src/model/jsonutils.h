#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <QVariantMap>

class JsonUtils
{
public:
    static QVariantMap bytesToJsonMap(const QByteArray& bytes);
    static QString updateUrl(const QVariantMap& jsonMap);
};

#endif // JSONUTILS_H
