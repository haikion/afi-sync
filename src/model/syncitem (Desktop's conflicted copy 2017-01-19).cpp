#include <limits>
#include <QTime>
#include "syncitem.h"
#include "debug.h"

QSettings* SyncItem::settings_ = nullptr;

SyncItem::SyncItem(const QString& name, TreeItem* parentItem):
    TreeItem(name, parentItem),
    name_(name),
    status_(SyncStatus::NO_SYNC_CONNECTION),
    eta_(Constants::MAX_ETA),
    fileSize_(0)
{
    if (settings_ == nullptr)
    {
        settings_ = new QSettings();
    }
}

QString SyncItem::checkText()
{
    QString rVal = ticked() ? "true" : "false";
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

QString SyncItem::fileSizeText() const
{
    if (fileSize_ == 0)
        return QString("??.?? MB");

    double size = fileSize_;
    int i = 0;
    QStringList list;
    list << "B" << "kB" << "MB" << "GB";

    for (i = 0; i < list.size() && size > 1024; ++i)
        size = size / double(1024);

    return QString::number(size, 'f', 2) + " " + list.at(i);
}

QString SyncItem::progressText()
{
    //In repository multiple MAX_ETA maybe summed so we use >=.
    if (eta_ >= Constants::MAX_ETA)
        return "??:??:??";

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

bool SyncItem::ticked() const
{
    return settings_->value(name() + "/checked", false).toBool();
}

void SyncItem::setTicked(bool checked)
{
    settings_->setValue(name() + "/checked", checked);
}

void SyncItem::checkboxClicked()
{
    setTicked(!ticked());
}

quint64 SyncItem::fileSize() const
{
    return fileSize_;
}

void SyncItem::setFileSize(const quint64& size)
{
    fileSize_ = size;
}

QSettings* SyncItem::settings() const
{
    return settings_;
}

void SyncItem::setStatus(const QString& status)
{
    status_ = status;
}

QString SyncItem::status() const
{
    return status_;
}
