#include "../../runningtime.h"
#include "../../debug.h"
#include "speedestimator.h"

const int64_t SpeedEstimator::AVG_CHECKING_SPEED = 20000000; //Bytes per sec

SpeedEstimator::SpeedEstimator():
    dX_(0), dT_(0)
{
}

bool SpeedEstimator::cutEsimation(const QString& key)
{
    return progresses_.remove(key) > 0;
}

unsigned SpeedEstimator::estimate(const QString& key, int64_t toCheck)
{
    unsigned t_2 = runningTimeMs();
    int64_t rVal = AVG_CHECKING_SPEED;
    if (toCheck < 0)
    {
        DBG << "ERROR: toCheck is negative. toCheck =" << toCheck;
        return AVG_CHECKING_SPEED;
    }
    if (progresses_.contains(key))
    {
        auto val = progresses_.value(key);
        int64_t x_1 = val.first;
        int64_t t_1 = val.second;

        if (x_1 > toCheck)
        {
            DBG << "ERROR: x_1 > toCheck. x_1 =" << x_1 << "toCheck =" << toCheck;
            return AVG_CHECKING_SPEED;
        }

        dX_ += x_1 - toCheck;
        dT_ += t_2 - t_1;
        rVal = 1000 * dX_ / dT_;
    }
    std::pair<long unsigned, unsigned> newVal(toCheck, t_2);
    progresses_[key] = newVal;
    return rVal;
}
