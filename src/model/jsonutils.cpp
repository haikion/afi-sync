#include <QJsonDocument>
#include "jsonutils.h"

QVariantMap JsonUtils::bytesToJsonMap(const QByteArray& bytes)
{
    QJsonDocument docElement = QJsonDocument::fromJson(bytes);
    if (docElement == QJsonDocument())
    {
        //Incorrect reply from network server
        return QVariantMap();
    }
    return qvariant_cast<QVariantMap>(docElement.toVariant());
}

QString JsonUtils::updateUrl(const QVariantMap& jsonMap)
{
    return qvariant_cast<QString>(jsonMap.value("updateUrl"));
}
