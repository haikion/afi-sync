#include "speedcalculator.h"

using namespace std::chrono_literals;

bool SpeedCalculator::update(int64_t downloaded, int64_t uploaded, libtorrent::time_point timepoint) {
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

int64_t SpeedCalculator::getDownloadSpeed()
{
    return downloadSpeed_;
}

int64_t SpeedCalculator::getUploadSpeed()
{
    return uploadSpeed_;
}

int64_t SpeedCalculator::calculateSpeed(int64_t prev, int64_t curr, libtorrent::time_point timepoint)
{
    const auto interval = lt::total_microseconds(timepoint - prevTimepoint_);
    if (interval <= 0) {
        return 0;
    }
    return ((curr - prev)*lt::microseconds(1s).count()) / interval;
}
