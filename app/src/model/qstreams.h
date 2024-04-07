#ifndef QSTREAMS_H
#define QSTREAMS_H
#include <ostream>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QList>

std::ostream& operator<<(std::ostream& outStream, const QString& qString);
std::ostream& operator<<(std::ostream& outStream, const QList<int>& qVector);
std::ostream& operator<<(std::ostream& outStream, const QStringList& qList);
std::ostream& operator<<(std::ostream& outStream, const QSet<QString>& qSet);

#endif // QSTREAMS_H
