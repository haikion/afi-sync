#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QString>

class DirectoryWatcher
{
public:
    DirectoryWatcher(const QString& fromPath = {},
                     const QString& toPath_ = {},
                     const QString& key = {},
                     qint64 totalWantedForce = -1);
    bool done();
    [[nodiscard]] QString key() const;
    [[nodiscard]] qint64 totalWanted() const;
    [[nodiscard]] qint64 totalWantedDone() const;

private:
    QString fromPath_;
    QString toPath_;
    qint64 totalWantedDone_{0};
    qint64 totalWanted_;
    QString key_;
};

#endif // DIRECTORYWATCHER_H
