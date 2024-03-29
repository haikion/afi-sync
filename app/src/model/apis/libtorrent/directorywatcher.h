#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QString>

class DirectoryWatcher
{
public:
    DirectoryWatcher(const QString& fromPath = "", const QString& toPath_ = "", const QString& key = "", const qint64 totalWantedForce = -1);
    bool done();
    QString key() const;
    qint64 totalWanted() const;
    qint64 totalWantedDone() const;

private:
    QString fromPath_;
    QString toPath_;
    qint64 totalWantedDone_;
    qint64 totalWanted_;
    QString key_;
};

#endif // DIRECTORYWATCHER_H
