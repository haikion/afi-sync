#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QVariant>
#include <QVariant>
#include <QObject>
#include <QList>
#include <QSet>
#include "repository.h"
#include "apis/isync.h"
#include "interfaces/ibandwidthmeter.h"
#include "jsonreader.h"

class TreeModel : public QObject, virtual public IBandwidthMeter
{
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = nullptr);
    ~TreeModel() override;

    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    QList<IRepository*> repositories() const;
    void moveFiles();
    void stopUpdates();

public slots:
    QString downloadStr() const override;
    QString uploadStr() const override;
    void updateSpeed();

signals:
    void repositoriesChanged(QList<IRepository*> repositories);

private slots:
    void update();
    void periodicRepoUpdate();

private:
    unsigned download_;
    unsigned upload_;
    QList<Repository*> repositories_;
    QTimer updateTimer_;
    ISync* sync_;
    QTimer repoUpdateTimer_;
    JsonReader jsonReader_;

    void postInit();
    QString bandwithString(int amount) const;
    QSet<Mod*> mods() const;
    void manageDeltaUpdates(const JsonReader& jsonReader);
    void createSync(const JsonReader& jsonReader);
    void updateRepositories();
    static QList<IRepository*> toIrepositories(const QList<Repository*>& repositories);
};

#endif // TREEMODEL_H
