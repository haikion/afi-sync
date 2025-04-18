#include "speedcalculator.h"
#include <libtorrent/time.hpp>

using namespace std::chrono_literals;

static int64_t calculateSpeed(int64_t prev, int64_t curr, int64_t interval)
{
    return ((curr - prev)*lt::milliseconds(1s).count()) / interval;
}

bool SpeedCalculator::update(int64_t downloaded, int64_t uploaded, libtorrent::time_point timepoint) {
    const int64_t timeDiff = lt::total_milliseconds(timepoint - prevTimepoint_);
    // Only calculate new speeds if the minimum interval has passed
    if (timeDiff >= lt::total_milliseconds(2s)) {
        auto ul = calculateSpeed(totalUploaded_, uploaded, timeDiff);
        auto dl = calculateSpeed(totalDownloaded_, downloaded, timeDiff);
        auto retVal = uploadSpeed_ != ul || downloadSpeed_ != dl;
        uploadSpeed_ = ul;
        downloadSpeed_ = dl;
        prevTimepoint_ = timepoint;
        totalDownloaded_ = downloaded;
        totalUploaded_ = uploaded;
        return retVal;
    }
    return false;
}

int64_t SpeedCalculator::getDownloadSpeed() const
{
    return downloadSpeed_;
}

int64_t SpeedCalculator::getUploadSpeed() const
{
    return uploadSpeed_;
}
