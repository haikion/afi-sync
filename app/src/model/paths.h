#pragma once

#include <QString>

/**
 * Reads Windows registery in order to discover default paths
 */
namespace Paths
{
    [[nodiscard]] QString arma3Path();
    [[nodiscard]] QString teamspeak3Path();
    [[nodiscard]] QString teamspeak3AppDataPath();
    [[nodiscard]] QString steamPath();
};
