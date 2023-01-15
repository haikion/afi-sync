#ifndef SPEEDCALCULATOR_H
#define SPEEDCALCULATOR_H

#include <libtorrent/time.hpp>
#include <QtGlobal>

class SpeedCalculator
{
public:
    SpeedCalculator() = default;
    bool update(qint64 downloaded, qint64 uploaded, libtorrent::time_point timepoint);
    qint64 getDownloadSpeed();
    qint64 getUploadSpeed();

private:
    qint64 totalDownloaded_{0};
    qint64 totalUploaded_{0};
    qint64 downloadSpeed_{0};
    qint64 uploadSpeed_{0};
    libtorrent::time_point prevTimepoint_;

    qint64 calculateSpeed(qint64 prev, qint64 curr, libtorrent::time_point timepoint);
};

#endif // SPEEDCALCULATOR_H
