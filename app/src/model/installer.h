#ifndef INSTALLER_H
#define INSTALLER_H

/*
 * Performs installation of files that are not inside @mod directory.
 */

#include <QDir>
#include "mod.h"

class Installer
{
public:
    static void install(const Mod* mod);

private:
    static const QString TS_DIR;
    static const QString USER_CONFIG_DIR;

    static bool install(const QDir& src, const QDir& dst);
};

#endif // INSTALLER_H
