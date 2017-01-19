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
    int64_t estimate(const QString& key, const int64_t toCheck);
    int64_t estimation() const;
    bool cutEsimation(const QString& key);

private:
    static const int64_t AVG_CHECKING_SPEED;

    int64_t dX_; //Total bytes checked
    int64_t dT_; //Total summed time in ms
    //Contains x and timestamp
    QHash<QString, std::pair<int64_t, int64_t>> progresses_;
};

#endif // SPEEDESTIMATOR_H
