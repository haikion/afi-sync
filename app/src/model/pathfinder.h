//Reads windows registery in order to discover default paths
#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <QString>

class PathFinder
{
public:
    [[nodiscard]] static QString arma3Path();
    [[nodiscard]] static QString teamspeak3Path();
    [[nodiscard]] static QString teamspeak3AppDataPath();
    [[nodiscard]] static QString steamPath();

private:
    static QString readRegPath(const QString& path, const QString& key);
    static void checkPath(const QString& path, const QString& name);
};

#endif // PATHFINDER_H
