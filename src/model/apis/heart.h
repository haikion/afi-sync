#ifndef HEART_H
#define HEART_H

#include <QObject>
#include <QTimer>
#include <QNetworkReply>

//An heart for the heart beat pattern.
class Heart : public QObject
{
    Q_OBJECT
public:
    explicit Heart(QObject* parent = 0, int maxDelay = 30);

    void beat(const QNetworkReply* reply = nullptr);
    void reset(int maxDelay = 30);

signals:
    void death();

private:
    QTimer timer_;
    int maxDelay_;
    int secondsToDeath_;

private slots:
    void process();
};

#endif // HEART_H
