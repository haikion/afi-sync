#include "versioncheckresult.h"
#include "afisynclogger.h"
#include "version.h"

using namespace Qt::StringLiterals;

void VersionCheckResult::readJson(const QVariantMap& json)
{
    latestVersion_ = json.value(u"latestVersion"_s).toString();
    versionUrl_ = json.value(u"versionUrl"_s).toString();
}

QString VersionCheckResult::latestVersion() const
{
    return latestVersion_;
}

QString VersionCheckResult::versionUrl() const
{
    return versionUrl_;
}

bool VersionCheckResult::updateAvailable() const {
    if (versionUrl_.isEmpty()) {
        LOG << "Version check disabled: version url not set";
        return false;
    }
    auto splits = latestVersion_.split('.');
    if (splits.size() != 2) {
        LOG << "Version check disabled: latestVersion not set";
        return false;
    }
    bool majorOk = false;
    auto major = splits.at(0).toInt(&majorOk);
    if (!majorOk) {
        LOG_WARNING << "Invalid major version in: " << latestVersion_;
        return false;
    }
    bool minorOk = false;
    auto minor = splits.at(1).toInt(&minorOk);
    if (!minorOk) {
        LOG_WARNING << "Invalid minor version in: " << latestVersion_;
        return false;
    }
    return major > MAJOR_VERSION || (major == MAJOR_VERSION && minor > MINOR_VERSION);
}

