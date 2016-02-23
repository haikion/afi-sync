//Reads windows registery in order to discover default paths
#ifndef PATHFINDER_H
#define PATHFINDER_H
#include <QString>

class PathFinder
{
public:
    static QString arma3Path();
    static QString arma3MyDocuments();
    static QString teamspeak3Path();
    static QString steamPath();

private:
    //static const QString DEFAULT_PATH;
    static QString readRegPath(const QString& path, const QString& key);
};

#endif // PATHFINDER_H
