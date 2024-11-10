#ifndef VERSIONCHECKRESULT_H
#define VERSIONCHECKRESULT_H

#include <QVariantMap>

class VersionCheckResult
{
public:
    VersionCheckResult() = default;

    void readJson(const QVariantMap& json);
    QString latestVersion() const;
    QString versionUrl() const;
    bool updateAvailable() const;

private:
    QString latestVersion_;
    QString versionUrl_;
};

#endif // VERSIONCHECKRESULT_H
