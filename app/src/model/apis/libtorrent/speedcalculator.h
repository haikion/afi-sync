#ifndef SPEEDCALCULATOR_H
#define SPEEDCALCULATOR_H

#include <libtorrent/time.hpp>

class SpeedCalculator
{
public:
    SpeedCalculator() = default;
    bool update(int64_t downloaded, int64_t uploaded, libtorrent::time_point timepoint);
    [[nodiscard]] int64_t getDownloadSpeed() const;
    [[nodiscard]] int64_t getUploadSpeed() const;

private:
    int64_t totalDownloaded_{0};
    int64_t totalUploaded_{0};
    int64_t downloadSpeed_{0};
    int64_t uploadSpeed_{0};
    libtorrent::time_point prevTimepoint_;

    int64_t calculateSpeed(int64_t prev, int64_t curr, libtorrent::time_point timepoint);
};

#endif // SPEEDCALCULATOR_H
