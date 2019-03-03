#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QDir>
#include <QString>

class DirectoryWatcher
{
public:
    DirectoryWatcher(const QString& fromPath = "", const QString& toPath_ = "", const QString& key = "", const qint64& totalWanted = -1);
    bool done();
    QString key() const;
    qint64 totalWanted() const;
    qint64 totalWantedDone() const;

private:
    QDir fromDir_;
    QString fromPath_;
    QString toPath_;
    qint64 totalWantedDone_;
    qint64 totalWanted_;
    QString key_;
};

#endif // DIRECTORYWATCHER_H
