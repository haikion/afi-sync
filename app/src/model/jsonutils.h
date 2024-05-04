#ifndef JSONUTILS_H
#define JSONUTILS_H

#include <QVariantMap>

class JsonUtils
{
public:
    [[nodiscard]] static QVariantMap bytesToJsonMap(const QByteArray& bytes);
    [[nodiscard]] static QString updateUrl(const QVariantMap& jsonMap);
};

#endif // JSONUTILS_H
