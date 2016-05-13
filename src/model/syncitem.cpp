#include <limits>
#include "syncitem.h"
#include "debug.h"

QSettings* SyncItem::settings_ = nullptr;

SyncItem::SyncItem(const QString& name, TreeItem* parentItem):
    TreeItem(name, parentItem),
    name_(name),
    status_(SyncStatus::NO_BTSYNC_CONNECTION),
    eta_(std::numeric_limits<int>::max())
{
    if (settings_ == nullptr)
    {
        settings_ = new QSettings();
    }
    setChecked(checked());
}

QString SyncItem::checkText()
{
    QString rVal = checked() ? "true" : "false";
    return rVal;
}

QString SyncItem::nameText()
{
    return name_;
}

QString SyncItem::statusText()
{
    return status();
}

QString SyncItem::progressText()
{
    QString rVal = QTime(0,0,0).addSecs(eta_).toString();
    return rVal;
}

QString SyncItem::startText()
{
    return "uploadText()";
}

int SyncItem::eta() const
{
    return eta_;
}

void SyncItem::setEta(const int& eta)
{
    eta_ = eta;
}

QString SyncItem::name() const
{
    return name_;
}

void SyncItem::setName(const QString& name)
{
    name_ = name;
}

bool SyncItem::checked() const
{
    return settings_->value(name() + "/checked", false).toBool();
}

void SyncItem::setChecked(bool checked)
{
    settings_->setValue(name() + "/checked", checked);
}

void SyncItem::checkboxClicked()
{
    setChecked(!checked());
}

void SyncItem::setStatus(const QString& status)
{
    status_ = status;
}

QString SyncItem::status() const
{
    return status_;
}
