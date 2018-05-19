#include "../debug.h"
#include "heart.h"

Heart::Heart(QObject* parent, int maxDelay) :
    QObject(parent),
    maxDelay_(maxDelay)
{
    LOG << "maxDelay =" << maxDelay;
    timer_.setInterval(1000);
    connect(&timer_, SIGNAL(timeout()), this, SLOT(process()));
    reset();
}

void Heart::reset(int maxDelay)
{
    LOG << "maxDelay =" << maxDelay;
    maxDelay_ = maxDelay;
    beat();
    timer_.start();
}

void Heart::stop()
{
    timer_.stop();
}

void Heart::beat(const QNetworkReply* reply)
{
    if (reply == nullptr || (reply->isFinished() && reply->error() == QNetworkReply::NoError))
    {
        secondsToDeath_ = maxDelay_;
    }
}

void Heart::process()
{
    --secondsToDeath_;
    if (secondsToDeath_ < 0)
    {
        LOG << "Death";
        emit death();
        timer_.stop();
    }
}

