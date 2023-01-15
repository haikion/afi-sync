#include "speedcalculator.h"

using namespace std::chrono_literals;

bool SpeedCalculator::update(qint64 downloaded, qint64 uploaded, libtorrent::time_point timepoint) {
    auto ul = calculateSpeed(totalUploaded_, uploaded, timepoint);
    auto dl = calculateSpeed(totalDownloaded_, downloaded, timepoint);
    auto retVal = uploadSpeed_ != ul || downloadSpeed_ != dl;
    totalDownloaded_ = downloaded;
    totalUploaded_ = uploaded;
    uploadSpeed_ = ul;
    downloadSpeed_ = dl;
    prevTimepoint_ = timepoint;
    return retVal;
}

qint64 SpeedCalculator::getDownloadSpeed()
{
    return downloadSpeed_;
}

qint64 SpeedCalculator::getUploadSpeed()
{
    return uploadSpeed_;
}

qint64 SpeedCalculator::calculateSpeed(qint64 prev, qint64 curr, libtorrent::time_point timepoint)
{
    const qint64 interval = lt::total_microseconds(timepoint - prevTimepoint_);
    if (interval <= 0) {
        return 0;
    }
    auto val = ((curr - prev)*lt::microseconds(1s).count()) / interval;
    return ((curr - prev)*lt::microseconds(1s).count()) / interval;
}
