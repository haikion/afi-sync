#ifndef QSTREAMS_H
#define QSTREAMS_H

#include <ostream>

#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

std::ostream& operator<<(std::ostream& outStream, const QString& qString);
std::ostream& operator<<(std::ostream& outStream, const QList<int>& qVector);
std::ostream& operator<<(std::ostream& outStream, const QStringList& qList);
std::ostream& operator<<(std::ostream& outStream, const QSet<QString>& qSet);

#endif // QSTREAMS_H
