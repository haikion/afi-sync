#ifndef DELETABLEDETECTOR_H
#define DELETABLEDETECTOR_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QFileInfo>
#include <QList>
#include "repository.h"

class DeletableDetector
{
public:
    static void printDeletables(const QList<Repository*> repositories);

private:
    static QSet<QString> activeMods(const Repository* repository);
    static QStringList activeMods(const QList<Repository*> repositories);
    static QList<QFileInfo> deletables(const QStringList& activeMods);
    static void printDeletables(const QList<QFileInfo>& deletables);
};

#endif // DELETABLEDETECTOR_H
