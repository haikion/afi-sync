#include "../../debug.h"
#include "../../runningtime.h"
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

int64_t SpeedEstimator::estimate(const QString& key, const int64_t toCheck)
{
    int64_t t_2 = runningTimeS();
    int64_t rVal = estimation();

    if (toCheck <= 0) //Sanity check TODO: Might be too defensive
    {
        LOG << "ERROR: toCheck is negative or zero. toCheck =" << toCheck;
        return rVal;
    }
    if (progresses_.contains(key))
    {
        auto val = progresses_.value(key);
        int64_t x_1 = val.first;
        int64_t t_1 = val.second;

        if (x_1 < toCheck) //Sanity check
        {
            LOG << "ERROR: x_1 < toCheck. x_1 =" << x_1 << "toCheck =" << toCheck;
            return rVal;
        }
        //t_1 == t_2, might happen because of the VeryCoarseTimer?
        if (t_1 >= t_2)
        {
            if (t_1 > t_2) //Sanity check, this should never happen. TODO: Might be too defensive
                LOG << "ERROR: t_1 > t_2, t_1 =" << t_1 << " t_2 =" << t_2;
            return rVal;
        }

        dX_ += x_1 - toCheck;
        dT_ += t_2 - t_1;

        rVal = 1000 * dX_ / dT_;
    }
    std::pair<int64_t, int64_t> newVal(toCheck, t_2);
    progresses_[key] = newVal;
    return rVal;
}

int64_t SpeedEstimator::estimation() const
{
    int64_t rVal = AVG_CHECKING_SPEED;
    if (dT_ > 10000  && dX_ > AVG_CHECKING_SPEED)
    {
        //Enough estimation data...
        rVal = 1000 * dX_ / dT_;
    }
    return rVal;
}
