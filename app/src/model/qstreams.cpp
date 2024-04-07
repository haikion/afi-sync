#include "qstreams.h"

std::ostream& operator<<(std::ostream& outStream, const QString& qString)
{
    outStream << qString.toStdString();
    return outStream;
}

std::ostream& operator<<(std::ostream& outStream, const QList<int>& qVector)
{
    outStream << " QVector[" << qVector.at(0);
    for (int i = 1; i < qVector.size(); ++i)
    {
        outStream << ", " << qVector.at(i);
    }
    outStream << "]";
    return outStream;
}

std::ostream& operator<<(std::ostream& outStream, const QStringList& qList)
{
    outStream << " [" << qList.at(0);
    for (int i = 1; i < qList.size(); ++i)
    {
        outStream << ", " << qList.at(i);
    }
    outStream << "]";
    return outStream;
}

std::ostream& operator<<(std::ostream& outStream, const QSet<QString>& qSet)
{
    outStream << qSet.values();
    return outStream;
}
