#include <QJsonDocument>
#include <QStringLiteral>

#include "jsonutils.h"

using namespace Qt::StringLiterals;

QVariantMap JsonUtils::bytesToJsonMap(const QByteArray& bytes)
{
    QJsonDocument docElement = QJsonDocument::fromJson(bytes);
    if (docElement == QJsonDocument())
    {
        //Incorrect reply from network server
        return {};
    }
    return qvariant_cast<QVariantMap>(docElement.toVariant());
}

QString JsonUtils::updateUrl(const QVariantMap& jsonMap)
{
    return qvariant_cast<QString>(jsonMap.value(u"updateUrl"_s));
}
