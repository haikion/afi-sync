//Calculates estimation for x's per ms. Works for example in checking speed estimation.

#ifndef SPEEDESTIMATOR_H
#define SPEEDESTIMATOR_H

#include <QString>
#include <QSet>

class SpeedEstimator
{
public:
    SpeedEstimator();

    //toCheck is remaining x's.
    unsigned estimate(const QString& key, int64_t toCheck);
    bool cutEsimation(const QString& key);

private:
    static const int AVG_CHECKING_SPEED;

    long unsigned dX_;
    long unsigned dT_; //ms
    //Contains x and timestamp
    QHash<QString, std::pair<int64_t, unsigned>> progresses_;
};

#endif // SPEEDESTIMATOR_H
